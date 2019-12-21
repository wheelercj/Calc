#include "Calc.h"

std::string Calc::calc(std::string input)
{
	/*
		Reads the input string and pushes values onto the appropriate stack. Values are popped off the stacks
		and evaluated whenever an operator of lower precedence is found following one of higher precedence.
	 */
	std::string result;

	try
	{
		formatInput(input);

		for (int i = 0; i < input.size();) // the index is incremented when value(s) are pushed onto a stack
		{
			if (isNumber(input[i]))
			{
				int numLength = getNumLength(input.substr(i, input.size() - i));
				nums.push(stod(input.substr(i, numLength)));
				i += numLength;
			}
			else if (isOp(input[i]))
			{

				if (ops.empty())
				{
					if (i == 0 && input[i] == '-') // then the minus is a negation operator
					{
						ops.push('n');
						i++;
					}
					else if (nums.empty() && input[i] == '!')
						throw "Invalid syntax";
					else
						ops.push(input[i++]);
				}
				else // follow the order of operations
				{
					/*	Order of ops:
						- Operators within parentheses
						- Factorial
						- Negation
						- Exponent
						- Multiplication and Division
						- Addition and Subtraction
						- Modulus
					*/

					if (input[i] == '(')
					{
						if (i > 0 && isNumber(input[i - 1]))
						{
							if (ops.top() == '^' || ops.top() == '*' || ops.top() == '/')
								pop();
							else
							{
								ops.push('*');
								ops.push(input[i++]);
							}
						}
						else
							ops.push(input[i++]);
					}
					else if (input[i] == ')')
					{
						while (!ops.empty() && ops.top() != '(')
							pop();
						if (ops.empty())
							throw "Invalid syntax";
						pop();
						i++;
					}
					else if (input[i] == '!')
					{
						ops.push(input[i++]);
						pop();
					}
					else if (input[i] == '^')
					{
						if (ops.top() == '!')
							pop();
						else
							ops.push(input[i++]);
					}
					else if (input[i] == '*' || input[i] == '/')
					{
						if (ops.top() == '^' || ops.top() == '*' || ops.top() == '/' || ops.top() == 'n' || ops.top() == '!')
							pop();
						else
							ops.push(input[i++]);
					}
					else if (input[i] == '+')
					{
						if (ops.top() == '^' || ops.top() == '*' || ops.top() == '/' || ops.top() == '+'
							|| ops.top() == '-' || ops.top() == 'n' || ops.top() == '!')
							pop();
						else
							ops.push(input[i++]);
					}
					else if (input[i] == '-')
					{
						if (i == 0 || isOp(input[i - 1])) // then the minus is a negation operator
						{
							if (ops.top() == '!')
								pop();
							else
							{
								ops.push('n');
								i++;
							}
						}
						else // then the minus is a subtraction operator
						{
							if (ops.top() == '^' || ops.top() == '*' || ops.top() == '/' || ops.top() == '+'
								|| ops.top() == '-' || ops.top() == 'n' || ops.top() == '!')
								pop();
							else
								ops.push(input[i++]);
						}
					}
					else if (input[i] == '%')
					{
						if (ops.top() == '(')
							ops.push(input[i++]);
						else
							pop();
					}
				}
			}
			else
				throw "Undefined character";
		}

		if (!input.size())
			throw " ";
		while (!ops.empty())
			pop();
		while (nums.size() > 1) // remaining numbers will be multiplied together
			pop();

		result = std::to_string(nums.top());
		formatOutput(result);
		if (result == "-0")
			result = "0";
	}
	catch (const char* error)
	{
		result = error;
	}
	catch (std::bad_alloc)
	{
		result = "Insufficient memory";
	}

	while (!nums.empty())
		nums.pop();
	while (!ops.empty())
		ops.pop();
	return result;
}

void Calc::formatInput(std::string& input)
{
	// remove all spaces from the input string
	for (int i = 0; i < input.size();)
	{
		if (input[i] == ' ')
			input.erase(i, 1);
		else
			i++;
	}

	// check for invalid syntax: multiple operators next to each other
	for (int i = 1; i < input.size(); i++)
	{
		if (isOp(input[i]) && input[i] != '-' && input[i] != '(' && input[i] != ')')
		{
			if (isOp(input[i - 1]) && input[i - 1] != ')' && (input[i - 1] != '!' || input[i - 1] == '!' && input[i] == '!'))
				throw "Invalid syntax";
		}
	}
}

void Calc::formatOutput(std::string& str)
{
	if (str == "inf")
		throw "Infinity";

	// remove any trailing zeros
	for (int i = str.size() - 1; i > 0; i--)
	{
		if (str[i] == '0')
			str.erase(i, 1);
		else if (str[i] == '.')
		{
			str.erase(i, 1);
			return;
		}
		else
			return;
	}
}

bool Calc::isNumber(char ch)
{
	if (ch >= '0' && ch <= '9' || ch == '.')
		return true;
	return false;
}

bool Calc::isOp(char ch)
{
	if (ch == '(' || ch == ')' || ch == '^' || ch == '*' || ch == '/' || ch == '+' || ch == '-' || ch == '!' || ch == '%')
		return true;
	return false;
}

int Calc::getNumLength(std::string str)
{
	bool periodFound = false;
	for (int i = 0; i < str.size(); i++)
	{
		if (str[i] == '.')
		{
			if (periodFound || str.size() == 1)
				throw "Invalid syntax";
			periodFound = true;
		}
		else if (!isNumber(str[i]))
		{
			if (i == 1 && str[0] == '.')
				throw "Invalid syntax";
			return i;
		}
	}

	return str.size();
}

void Calc::pop()
{
	if (nums.empty())
		throw "Invalid syntax";

	char op;
	if (ops.empty())
		op = '*';
	else
	{
		if (ops.top() == '(')
		{
			ops.pop();
			return;
		}
		else if (ops.top() == 'n')
		{
			// negate nums.top()
			double temp = nums.top();
			nums.pop();
			temp *= -1;
			nums.push(temp);
			ops.pop();
			return;
		}
		else if (ops.top() == '!')
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
				for (int i = n; i > 0; i--)
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
		throw "Invalid syntax";

	double num2 = nums.top();
	nums.pop();
	double num1 = nums.top();
	nums.pop();

	switch (op)
	{
	case '^':
		if (num1 == 0 && num2 == 0)
			throw "Indeterminate";
		else if (num1 < 0 && num2 < 1 && (int)pow(num2, -1) % 2 == 0)
			throw "Imaginary";
		nums.push(pow(num1, num2));
		break;
	case '*':
		nums.push(num1 * num2);
		break;
	case '/':
		if (num2 == 0)
		{
			if (num1 == 0)
				throw "Indeterminate";
			throw "Infinity";
		}
		nums.push(num1 / num2);
		break;
	case '+':
		nums.push(num1 + num2);
		break;
	case '-':
		nums.push(num1 - num2);
		break;
	case '%':
		if (num2 == 0)
			throw "Undefined";
		nums.push(fmod(num1, num2));
	}
}