#include "Calc.h"
#include "Debug.hpp"
#include <vector>

Calc::Calc()
{
}

Calc::Calc(Calc* other)
{
	vars = other->vars;
	macros = other->macros;
	funcs = other->funcs;
	Precision = other->Precision + 5;
}

std::string Calc::calc(std::string input)
{
	std::string result;
	bool assigning = false;
	
	while (!nums.empty())
		nums.pop();
	while (!ops.empty())
		ops.pop();

	try
	{
		validateInput(input);
		assignmentFormat(input);
		result = evaluate(input);

		while (!varsBeingDefined.empty())
		{
			assigning = true;
			setSymbol<double>(vars, varsBeingDefined.top(), stold(result));
			varsBeingDefined.pop();
		}

		if (assigning)
			return "";
		formatOutput(result, Precision);
	}
	catch (const char* error)
	{
		result = error;
	}
	catch (std::string error)
	{
		result = error;
	}
	catch (std::bad_alloc)
	{
		result = "Insufficient memory";
	}

	return result;
}

void Calc::resetSymbols()
{
	vars = Symbols::defaultVars;
	macros = Symbols::defaultMacros;
	funcs = Symbols::defaultFuncs;
}

// check for multiple operators or periods next to each other
void Calc::validateInput(std::string& input)
{
	for (int i = 1; i < input.size(); i++)
	{
		char ch1 = input[i - 1],
			ch2 = input[i];

		if (ch1 == '.' && ch2 == '.')
		{
			std::string message = "Invalid syntax: periods next to each other";
			LOG(message);
			throw message;
		}
		else if (isOp(ch2) && ch2 != '-' && ch2 != '(' && ch2 != ')' && ch2 != '=')
		{
			if (isOp(ch1) && ch1 != ')' && (ch1 != '!' || ch1 == '!' && ch2 == '!' && i < input.size() - 1 && input[i + 1] != '='))
			{
				std::string message = "Invalid syntax: ";
				message += ch1;
				message += ch2;
				LOG(message);
				throw message;
			}
		}
	}
}

void Calc::formatOutput(std::string& str, int precision)
{
	// adjust output precision
	std::stringstream ss;
	ss.setf(std::ios::fixed);
	ss.precision(precision);
	ss << stold(str);
	str = ss.str();

	// remove any trailing zeros
	for (int i = str.size() - 1; i > 0; i--)
	{
		if (str[i] == '0')
			str.erase(i, 1);
		else if (str[i] == '.')
		{
			str.erase(i, 1);
			break;
		}
		else
			break;
	}

	if (str == "inf")
		str = "Infinity";
	else if (str == "-0")
		str = "0";
}

// search for and handle any assignment operator(s)
void Calc::assignmentFormat(std::string& input)
{
	while (true) // multiple simultaneous variable assignments allowed, e.g. "a=b=c=5"
	{
		bool assigning = false;
		int eqPos = 0;
		for (; eqPos < input.size(); eqPos++)
		{
			if (input[eqPos] == '=')
			{
				// either input[eqPos] is an assignment operator or there isn't a valid one
				if (eqPos == 0)
					return;
				if (eqPos == input.size() - 1 || input[eqPos + 1] == '=')
					return;
				if (eqPos >= 1)
				{
					char ch = input[eqPos - 1];
					if (ch == '>' || ch == '<' || ch == '!')
						return;
				}

				assigning = true;
				break;
			}
		}

		if (!assigning)
			return;

		int macroNameSize = findMacroNameSize(input, eqPos);
		readSymbolDefinition(input, eqPos, macroNameSize);
	}
}

// returns zero if the symbol is a variable
int Calc::findMacroNameSize(std::string& input, int eqPos)
{
	bool alphaFound = false;
	bool spaceAfterAlpha = false;

	for (int i = 0; i < eqPos; i++)
	{
		if (isAlpha(input[i]))
		{
			alphaFound = true;
			if (spaceAfterAlpha)
			{
				std::string message = "Invalid space before assignment operator";
				LOG(message);
				throw message;
			}
		}
		else if (input[i] == ' ')
		{
			if (alphaFound)
				spaceAfterAlpha = true;
		}
		else if (input[i] == '(')
		{
			if (spaceAfterAlpha)
			{
				std::string message = "Invalid space before parameter(s)";
				LOG(message);
				throw message;
			}
			if (!alphaFound)
			{
				std::string message = "Missing symbol name before assignment operator";
				LOG(message);
				throw message;
			}

			return i;
			break;
		}
		else
		{
			std::string message = "Invalid character before assignment operator: ";
			message += input[i];
			LOG(message);
			throw message;
		}
	}

	if (!alphaFound)
	{
		std::string message = "Missing symbol name before assignment operator";
		LOG(message);
		throw message;
	}

	return 0;
}

void Calc::readSymbolDefinition(std::string& input, int eqPos, int macroNameSize)
{
	if (!macroNameSize) // then a variable is being defined
	{
		std::string varName = input.substr(0, eqPos);
		input.erase(0, eqPos + 1);
		removeEdgeSpaces(varName);
		varsBeingDefined.push(varName);
	}
	else // then a macro is being defined
	{
		if (varsBeingDefined.size())
		{
			std::string message = "Multiple simultaneous definitions are only possible with variables";
			LOG(message);
			throw message;
		}

		std::string macroName = input.substr(0, macroNameSize);
		removeEdgeSpaces(macroName);

		std::string paramStr = input.substr(macroNameSize, eqPos - macroNameSize + 1);
		std::vector<std::string> params = readParams(paramStr);
		for (int i = 0; i < params.size(); i++)
			removeEdgeSpaces(params[i]);

		std::string formula = input.substr(eqPos + 1, input.size() - eqPos);
		removeEdgeSpaces(formula);

		setSymbol<Macro>(macros, macroName, { params, formula });
		throw "";
	}
}

std::vector<std::string> Calc::readParams(std::string str)
{
	std::vector<std::string> params;
	bool alphaFound = false;
	bool spaceAfterAlpha = false;
	for (int i = 0, j = 1; i < str.size(); i++)
	{
		if (isAlpha(str[i]))
		{
			alphaFound = true;
			if (spaceAfterAlpha)
			{
				std::string message = "Invalid space in parameter name";
				LOG(message);
				throw message;
			}
		}
		else if (str[i] == ' ')
		{
			if (alphaFound)
				spaceAfterAlpha = true;
		}
		else if (str[i] == ',' || str[i] == ')')
		{
			std::string param = str.substr(j, i - j);
			if (param == "")
			{
				std::string message = "Invalid syntax: unnamed parameter";
				LOG(message);
				throw message;
			}
			params.push_back(param);
			j = i + 1;
			alphaFound = false;
			spaceAfterAlpha = false;

			if (str[i] == ')')
				break;
		}
		else if (isOp(str[i]))
		{
			if (str[i] != '(' || i > 0)
			{
				std::string message = "Invalid character in parameter list: ";
				message += str[i];
				LOG(message);
				throw message;
			}
		}
		else
		{
			std::string message = "Invalid character in parameter list: ";
			message += str[i];
			LOG(message);
			throw message;
		}
	}

	return params;
}

void Calc::removeEdgeSpaces(std::string& str)
{
	while (str[0] == ' ')
		str.erase(0, 1);
	while (str[str.size() - 1] == ' ')
		str.erase(str.size() - 1, 1);
}

// read the input string and determine what to do with each part of it
std::string Calc::evaluate(std::string input)
{
	std::string result,
		temp = "";
	lastTypePushed = NONE;

	for (int i = 0; i < input.size();) // the index is incremented when value(s) are pushed onto a stack
	{
		if (isNum(input[i]))
		{
			if (lastTypePushed == NUM)
				input.insert(i, "*");
			else
			{
				int numSize = getNumSize(input.substr(i, input.size() - i));
				nums.push(stold(input.substr(i, numSize)));
				i += numSize;
				lastTypePushed = NUM;
			}
		}
		else if (input[i] == ' ')
			i++;
		else if (isAlpha(input[i]))
		{
			int alphaSize = getAlphaSize(input.substr(i, input.size() - i));
			if (!getSymbolValue(input, i, alphaSize))
			{
				std::string message = "Undefined alpha character(s)";
				LOG(message);
				throw message;
			}
		}
		else if (isOp(input[i]))
			readOp(input, i);
		else
		{
			std::string message = "Undefined character: ";
			message += input[i];
			LOG(message);
			throw message;
		}
	}

	bool emptyString = true;
	for (int i = 0; i < input.size(); i++)
	{
		if (input[i] != ' ')
			emptyString = false;
	}
	if (emptyString)
		throw "";

	while (!ops.empty())
		pop();
	while (nums.size() > 1) // remaining numbers will be multiplied together
		pop();

	std::stringstream ss;
	ss.setf(std::ios::fixed);
	int p = Precision + 5;
	ss.precision(p);
	ss << nums.top();
	result = ss.str();
	setSymbol<double>(vars, "ans", stold(result));
	return result;
}

// pop and evaluate number(s) and/or operator(s)
void Calc::pop()
{
	if (nums.empty())
	{
		std::string message = "Error: not enough operands for the given operators";
		LOG(message);
		throw message;
	}

	std::string op;
	if (ops.empty())
		op = "*";
	else
	{
		if (ops.top() == "(")
		{
			ops.pop();
			return;
		}
		else if (ops.top() == "negate")
		{
			double temp = nums.top();
			nums.pop();
			temp *= -1;
			nums.push(temp);
			ops.pop();
			return;
		}
		else if (ops.top() == "!")
		{
			double n = nums.top(),
				total = 1,
				temp = n;
			nums.pop();

			if (n < 0)
				throw "Complex infinity";
			while (temp > 1)
				temp -= 1;
			if (temp < 1 && temp > 0)
				throw "Undefined";
			else if (n == 0)
				total = 1; // 0!=1 by convention
			else
			{
				for (int i = n; i > 1; i--)
					total *= i;
			}

			nums.push(total);
			ops.pop();
			return;
		}
		else
		{
			op = ops.top();
			ops.pop();
		}
	}

	if (nums.size() == 1)
	{
		std::string message = "Error: not enough operands for the given operators";
		LOG(message);
		throw message;
	}

	double num2 = nums.top();
	nums.pop();
	double num1 = nums.top();
	nums.pop();

	if (op == "^")
	{
		if (num1 == 0 && num2 == 0)
			throw "Indeterminate: 0^0";
		else if (num1 < 0 && num2 < 1 && (int)pow(num2, -1) % 2 == 0)
			throw "Imaginary: even root of a negative";
		nums.push(pow(num1, num2));
	}
	else if (op == "*")
		nums.push(num1 * num2);
	else if (op == "/")
	{
		if (num2 == 0)
		{
			if (num1 == 0)
				throw "Indeterminate: 0/0";
			throw "Infinity: n/0";
		}
		nums.push(num1 / num2);
	}
	else if (op == "+")
		nums.push(num1 + num2);
	else if (op == "subtract")
		nums.push(num1 - num2);
	else if (op == "%")
	{
		if (num2 == 0)
			throw "Undefined: n%0";
		nums.push(fmod(num1, num2));
	}
	else if (op == "==")
		nums.push(num1 == num2);
	else if (op == "!=")
		nums.push(num1 != num2);
	else if (op == ">=")
		nums.push(num1 >= num2);
	else if (op == "<=")
		nums.push(num1 <= num2);
	else if (op == ">")
		nums.push(num1 > num2);
	else if (op == "<")
		nums.push(num1 < num2);
	else
	{
		std::string message = "Undefined operator: ";
		message += op;
		LOG(message);
		throw message;
	}
}

bool Calc::isNum(char ch)
{
	if (ch >= '0' && ch <= '9' || ch == '.')
		return true;
	return false;
}

bool Calc::isAlpha(char ch)
{
	ch = tolower(ch);
	if (ch >= 'a' && ch <= 'z' || ch == '_')
		return true;
	return false;
}

bool Calc::isOp(char ch)
{
	std::string validOps = "()^*/+-!%<>=";
	if (validOps.find(ch) != std::string::npos)
		return true;
	return false;
}

int Calc::getNumSize(std::string str)
{
	bool periodFound = false;
	for (int i = 0; i < str.size(); i++)
	{
		if (str[i] == '.')
		{
			if (periodFound)
			{
				std::string message = "Error: multiple periods in one number";
				LOG(message);
				throw message;
			}
			if (str.size() == 1)
			{
				std::string message = "Invalid use of a period";
				LOG(message);
				throw message;
			}

			periodFound = true;
		}
		else if (!isNum(str[i]))
		{
			if (i == 1 && str[0] == '.')
			{
				std::string message = "Invalid use of a period";
				LOG(message);
				throw message;
			}

			return i;
		}
	}

	return str.size();
}

int Calc::getAlphaSize(std::string str)
{
	int i = 1;
	for (; i < str.size(); i++)
	{
		if (!isAlpha(str[i]))
			break;
	}

	return i;
}

int Calc::getOpSize(std::string str)
{
	if (str.size() > 1)
	{
		if (str[0] == '!' || str[0] == '=' || str[0] == '>' || str[0] == '<')
		{
			if (str[1] == '=' && !(str.size() > 2 && str[2] == '='))
				return 2;
		}
	}

	return 1;
}

void Calc::readOp(std::string input, int& pos)
{
	int opSize = getOpSize(input.substr(pos, input.size() - pos));
	std::string newOp = input.substr(pos, opSize);

	if (ops.empty())
	{
		if (newOp == "-")
		{
			if (lastTypePushed == NONE)
				ops.push("negate");
			else
				ops.push("subtract");
			lastTypePushed = OP;
		}
		else if (nums.empty() && newOp == "!")
		{
			std::string message = "Error: not enough operands for the given operators";
			LOG(message);
			throw message;
		}
		else
		{
			ops.push(newOp);
			lastTypePushed = OP;
		}

		pos += opSize;
	}
	else // if (!ops.empty())
	{
		if (newOp == "(")
		{
			if (pos > 0 && lastTypePushed == NUM)
			{
				if (ops.top() == "^" || ops.top() == "*" || ops.top() == "/")
					pop();
				else
				{
					ops.push("*");
					ops.push(input.substr(pos, 1));
					pos++;
					lastTypePushed = OP;
				}
			}
			else
			{
				ops.push(newOp);
				pos++;
				lastTypePushed = OP;
			}
		}
		else if (newOp == ")")
		{
			while (!ops.empty() && ops.top() != "(")
				pop();
			if (ops.empty())
			{
				std::string message = "Error: missing opening parenthesis";
				LOG(message);
				throw message;
			}
			pop();
			pos++;
		}
		else if (newOp == "!" || newOp == "*" || newOp == "/" || newOp == "+"
			|| newOp == "%" || newOp == "==" || newOp == ">=" || newOp == "<="
			|| newOp == "!=" || newOp == ">" || newOp == "<")
		{
			if (hasPrecedence(newOp))
			{
				ops.push(newOp);
				pos += opSize;
				lastTypePushed = OP;
			}
			else
				pop();
		}
		else if (newOp == "^")
		{
			if (ops.top() == "^" || hasPrecedence(newOp))
			{
				ops.push(newOp);
				pos += opSize;
				lastTypePushed = OP;
			}
			else
				pop();
		}
		else if (newOp == "-")
		{
			if (lastTypePushed == NONE || lastTypePushed == OP)
				newOp = "negate";
			else
				newOp = "subtract";

			if (newOp == "negate" || hasPrecedence(newOp))
			{
				ops.push(newOp);
				pos++;
				lastTypePushed = OP;
			}
			else
				pop();
		}
		else
		{
			std::string message = "Undefined operator: ";
			message += newOp;
			LOG(message);
			throw message;
		}
	}
}

bool Calc::hasPrecedence(std::string op1)
{
	std::string op2 = ops.top();
	std::vector<std::string> order = { "!", "^", "negate", "*", "+", "%", "==", "(" };

	// Replace some ops with others locally so that the order vector does not imply
	// that some ops with the same precedence have different precedences.
	if (op1 == "/")
		op1 = "*";
	else if (op1 == "subtract")
		op1 = "+";
	else if (op1 == ">=" || op1 == "<=" || op1 == "!=" || op1 == ">" || op1 == "<")
		op1 = "==";
	if (op2 == "/")
		op2 = "*";
	else if (op2 == "subtract")
		op2 = "+";
	else if (op2 == ">=" || op2 == "<=" || op2 == "!=" || op2 == ">" || op2 == "<")
		op2 = "==";

	for (int i = 0; i < order.size(); i++)
	{
		if (order[i] == op2)
			return false;
		if (order[i] == op1)
			return true;
	}
}

template<class T>
void Calc::setSymbol(std::unordered_map<std::string, T>& hashTable, std::string newName, T newSymbol)
{
	// erase any existing symbol with the given name
	vars.erase(newName);
	macros.erase(newName);
	funcs.erase(newName);
	
	hashTable.emplace(newName, newSymbol);
}

bool Calc::getSymbolValue(std::string& input, int alphaPos, int alphaSize)
{
	/*
		This function finds the name of one defined symbol within a string of alpha characters
		in the input string, and replaces the name with its value. There may be multiple symbols
		named in the alpha string with no spaces or anything else between them. Precedence is
		given to symbols with longer names, and to symbols further to the left of the string.
		All functions and macros must be immediately followed by an opening parenthesis.
	*/

	// for each substring size of the alpha string
	for (int size = alphaSize; size > 0; size--)
	{
		// for each substring position of the alpha string
		for (int pos = alphaPos; pos + size <= alphaPos + alphaSize; pos++)
		{
			std::string substr = input.substr(pos, size);

			// Variables
			std::unordered_map<std::string, double>::iterator it = vars.find(substr);
			if (it != vars.end())
			{
				// replace the variable name with its value
				input.erase(pos, size);
				std::stringstream ss;
				ss.setf(std::ios::fixed);
				int p = Precision + 5;
				ss.precision(p);
				ss << " " << it->second << " ";
				input.insert(pos, ss.str());
				return true;
			}
			
			// Macros
			std::unordered_map<std::string, Macro>::iterator it2 = macros.find(substr);
			if (it2 != macros.end())
			{
				input.erase(pos, size);
				std::vector<std::string> args = readArgs(input, pos);

				if (it2->first == "help")
				{
					if (args.size() && args[0] != "")
						throw help(args[0]);
					throw help();
				}
				if (it2->first == "setprecision")
				{
					if (args.size() == 1)
					{
						setprecision(stold(args[0]));
						throw "";
					}
					else
					{
						std::string message = "setprecision requires one argument";
						LOG(message);
						throw message;
					}
				}
				if (it2->first == "rand")
					throw random();

				if (args.size() != it2->second.getParamVect().size())
				{
					std::string message = "Error: ";
					message += std::to_string(it2->second.getParamVect().size());
					message += " argument(s) expected for ";
					message += it2->first;
					LOG(message);
					throw message;
				}

				// evaluate each argument
				Calc c2(this);
				for (int i = 0; i < args.size(); i++)
				{
					args[i] = c2.calc(args[i]);

					// rethrow messages
					if (args[i] == "")
					{
						std::string message = "Invalid syntax";
						LOG(message);
						throw message;
					}
					for (int j = 0; j < args[i].size(); j++)
					{
						if (isAlpha(args[i][j]))
						{
							std::string message = args[i];
							LOG(message);
							throw message;
						}
					}
				}

				// expand the macro and insert its return value
				input.insert(pos, " " + it2->second(args) + " ");
				return true;
			}

			// Functions
			std::unordered_map<std::string, Function>::iterator it3 = funcs.find(substr);
			if (it3 != funcs.end())
			{
				input.erase(pos, size);
				std::vector<std::string> args = readArgs(input, pos);
				if (args.size() != 1)
				{
					std::string message = "Error: expected one argument for ";
					message += it3->first;
					LOG(message);
					throw message;
				}

				// evaluate each argument
				Calc c2(this);
				for (int i = 0; i < args.size(); i++)
				{
					args[i] = c2.calc(args[i]);
					
					// rethrow messages
					if (args[i] == "")
					{
						std::string message = "Invalid syntax";
						LOG(message);
						throw message;
					}
					for (int j = 0; j < args[i].size(); j++)
					{
						if (isAlpha(args[i][j]))
						{
							std::string message = args[i];
							LOG(message);
							throw message;
						}
					}
				}

				// call the function and insert its return value
				std::string result = it3->second(args);
				if (result == "-nan(ind)")
					throw "Imaginary";
				input.insert(pos, "(" + result + ")");
				return true;
			}
		}
	}

	// the character(s) are undefined
	return false;
}

std::vector<std::string> Calc::readArgs(std::string& input, int pos)
{
	int parentheses = 0;

	if (input[pos] != '(')
	{
		std::string message = "Error: expected '(' after function name";
		LOG(message);
		throw message;
	}

	// get the arguments
	std::vector<std::string> args;
	for (int j = pos + 1, k = j, m = j - 1; ; j++)
	{
		if (j > input.size())
		{
			std::string message = "Invalid syntax";
			LOG(message);
			throw message;
		}
		if (j == input.size())
		{
			args.push_back(input.substr(k, j - k));
			input.erase(pos, j - m + 1);
			break;
		}
		if (input[j] == '(')
			parentheses++;
		if (input[j] == ')')
		{
			if (parentheses)
				parentheses--;
			else
			{
				args.push_back(input.substr(k, j - k));
				input.erase(pos, j - m + 1);
				break;
			}
		}
		if (input[j] == ',')
		{
			args.push_back(input.substr(k, j - k));
			j++;
			k = j;
		}
	}

	return args;
}

// throw info about all symbols
std::string Calc::help()
{
	std::string message = "";

	message += "\n Variables:";
	std::unordered_map<std::string, double>::iterator it;
	for (it = vars.begin(); it != vars.end(); it++)
	{
		std::stringstream ss;
		ss.setf(std::ios::fixed);
		int p = Precision + 5;
		ss.precision(p);
		ss << it->second;
		std::string num = ss.str();
		formatOutput(num, Precision + 5);
		message += "\n\t " + it->first + " = " + num;
	}

	message += "\n Functions:\t";
	std::unordered_map<std::string, Function>::iterator it3;
	int i = 0;
	for (it3 = funcs.begin(); it3 != funcs.end(); it3++, i++)
	{
		if (i % 10 == 0)
			message += "\n\t";
		message += it3->first + ", ";
	}

	message += "\n Macros:";
	std::unordered_map<std::string, Macro>::iterator it2;
	for (it2 = macros.begin(); it2 != macros.end(); it2++)
		message += "\n\t " + it2->first + "(" + it2->second.getParamStr() + ")" + " = " + it2->second.getFormula();

	return message;
}

// throw info about one symbol
std::string Calc::help(std::string name)
{
	std::string message = "";

	std::unordered_map<std::string, double>::iterator it = vars.find(name);
	if (it != vars.end())
	{
		std::stringstream ss;
		ss.setf(std::ios::fixed);
		int p = Precision + 5;
		ss.precision(p);
		ss << it->second;
		std::string num = ss.str();
		formatOutput(num, Precision + 5);
		message += "Variable " + it->first + " = " + num;
		return message;
	}

	std::unordered_map<std::string, Macro>::iterator it2 = macros.find(name);
	if (it2 != macros.end())
	{
		message += "Macro " + it2->first + "(" + it2->second.getParamStr() + ")" + " = " + it2->second.getFormula();
		return message;
	}

	std::unordered_map<std::string, Function>::iterator it3 = funcs.find(name);
	if (it3 != funcs.end())
		return "C++ Function";

	std::string errorMessage = "Undefined character(s)";
	LOG(errorMessage);
	throw errorMessage;
}

void Calc::setprecision(int num)
{
	Precision = num;
}

std::string Calc::random()
{
	srand((unsigned)time(0));
	double r = (rand() % 101) / 100.0;
	std::stringstream ss;
	ss.setf(std::ios::fixed);
	int p = Precision + 5;
	ss.precision(p);
	ss << r;
	return ss.str();
}
