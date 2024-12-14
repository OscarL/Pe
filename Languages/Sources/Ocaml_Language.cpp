// Based on D_Language.cpp and Cpp_Language.cpp, modified for OCaml, in 2024, by Oscar Lesta.

#include "CLanguageAddOn.h"
#include "HColorUtils.h"

const char	kLanguageName[]			= "OCaml";
const char	kLanguageExtensions[]	= "ml;mli";
const char	kLanguageCommentStart[]	= "(*";
const char	kLanguageCommentEnd[]	= "*)";
const char	kLanguageKeywordFile[]	= "keywords.ocaml";
const int16	kInterfaceVersion		= 2;

enum {
	START		= 0x00,
	IDENTIFIER,
	IDENTIFIER_CAPITALIZED,
	COMMENT,			// "(* *)"	Block Comments
	CHAR_CONST,		// 'a', '\'', 'xA9', '\169'
	STRING1,			// "whatever"
//	NAMING_LABEL,		//
	NUMERIC,	// [0-9], but also 0x, 0o, 0b notation. int32 -> xxxl, int64-> xxxL, native int: xxxn
	NUMERIC_FLOAT,		// float literal: 3.141, 3.141_592_653
	OPERATOR
};


static inline bool
isOperator(char c)
{
	switch (c) {
		// infix-symbol:
		case '#': return true;
		// "operator-char:
		case '~': return true;
		case '!': return true;
		case '?': return true;
		case '%': return true;
		case '<': return true;
		case ':': return true;
		case '.': return true;
		// "core-operator-char:
		case '$': return true;
		case '&': return true;
		case '*': return true;
		case '+': return true;
		case '-': return true;
		case '/': return true;
		case '=': return true;
		case '>': return true;
		case '@': return true;
		case '^': return true;
		case '|': return true;
		default: return false;
	}
}

bool isNumeric(char c)
{
	if (c >= '0' && c <= '9')
		return true;

	return false;
}

bool isHexNum(char c)
{
	if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))
		return true;

	return false;
}

bool isOctNum(char c)
{
	if (c >= '0' && c <= '8')
		return true;

	return false;
}

bool isBinNum(char c)
{
	if (c == '0' || c == '1')
		return true;

	return false;
}


bool find_comment_start(const char *text, int32 len, int32& offset)
{
	for (int32 i = 0; i < len; i++)
	{
		if (text[i] == '(')
		{
			if (i + 1 < len)
			{
				if (text[i + 1] == '*')
				{
					offset = i;
					return true;
				}
			}
		}
	}
	return false;
}

bool find_comment_end(const char *text, int32 len, int32& offset)
{
	for (int32 i = 0; i < len; i++)
	{
		if (text[i] == '*')
		{
			if (i + 1 < len && text[i + 1] == ')')
			{
				offset = i;
				return true;
			}
		}
	}
	return false;
}


_EXPORT void
ColorLine(CLanguageProxy& proxy, int32& state)
{
	int32 size = proxy.Size();

	if (size <= 0)
		return;

	switch (state) {
		case COMMENT:	proxy.SetColor(0, kColorComment1); break;
		default:		proxy.SetColor(0, kColorText);     break;
	}

	const char* text = proxy.Text();

	int32 i = 0, s = 0, kws = 0, cc_cnt = 0, esc = 0;
	char c;

	bool leave = false;
	bool floating_point = false;
	bool hex_num = false;

	while (!leave)
	{
		c = (i++ < size) ? text[i - 1] : 0;

		switch (state) {
			case START:
				s = i - 1;
				proxy.SetColor(s, kColorText);

				if (isalpha(c) || c == '_') {
					kws = proxy.Move(c, 1);
					if (isupper(c))
						state = IDENTIFIER_CAPITALIZED;
					else
						state = IDENTIFIER;
				}
				else if (c == '(' && text[i] == '*') {
					i++;
					state = COMMENT;
				}
				else if (c == '"') {
					state = STRING1;
				}
				else if (c == '\'') {
					state = CHAR_CONST;
					cc_cnt = 0;
				}
				else if (isdigit(c)) {
					state = NUMERIC;
				}
				else if (isOperator(c)) {
					state = OPERATOR;
				}
				else if (c == '\n' || c == 0) {
					leave = true;
				}
			break;

			case COMMENT:
				if ((s == 0 || i > s + 1) && c == '*' && text[i] == ')') {
					proxy.SetColor(s, kColorComment1);
					i++;
					state = START;
				} else if (c == 0 || c == '\n') {
					proxy.SetColor(s, kColorComment1);
					leave = true;
				}
			break;

			case IDENTIFIER_CAPITALIZED:
				if (!isalnum(c) && c != '_') {
					int32 kwc;
					proxy.SetColor(s, kColorKeyword2);
					i--;
					state = START;
				}
			break;

			case IDENTIFIER:
				if (!isalnum(c) && c != '_') {
					int32 kwc;

					if (i > s + 1 && (kwc = proxy.IsKeyword(kws)) != 0) {
						switch (kwc) {
							case 1:	proxy.SetColor(s, kColorKeyword1);  break;
							case 2:	proxy.SetColor(s, kColorUserSet1); break;
							case 3:	proxy.SetColor(s, kColorUserSet2); break;
							case 4:	proxy.SetColor(s, kColorUserSet3); break;
							case 5:	proxy.SetColor(s, kColorUserSet4); break;
						}
					}
					else
						proxy.SetColor(s, kColorText);

					i--;
					state = START;
				}
				else if (kws)
					kws = proxy.Move((int)(unsigned char) c, kws);
			break;

			case STRING1:
				if (c == '"' && !esc) {
					proxy.SetColor(s, kColorString1);
					state = START;
				} else if (c == '\n' || c == 0) {
					if (text[i - 2] == '\\' && text[i - 3] != '\\') {
						proxy.SetColor(s, kColorString1);
					} else {
						proxy.SetColor(s, kColorText);
						state = START;
					}
					leave = true;
				}
				else
					esc = !esc && (c == '\\');
			break;

			case CHAR_CONST:
				if (c == '\t' || c == '\n' || c == 0 ||
					(c == '\'' && !esc && (cc_cnt == 0 || cc_cnt > 8)))
				{
					// invalid char constant - either invalid char or too short/long
					proxy.SetColor(s, kColorText);
					state = START;
				} else if (c == '\'' && !esc) {
					proxy.SetColor(s, kColorCharConst);
					state = START;
				} else {
					if (!esc)
						cc_cnt++;

					esc = !esc && (c == '\\');
				}
			break;

			case NUMERIC:
				proxy.SetColor(s, kColorNumber1);
				if (!(isdigit(text[i - 1]) || (hex_num && isxdigit(text[i - 1]))))
				{
					if (text[i - 1] == '.' && floating_point == false && hex_num == false)
						floating_point = true;
					else if (text[i - 1] == 'x' && hex_num == false && floating_point == false)
						hex_num = true;
					else {
						i--;
						hex_num = false;
						state = START;
					}
				}
			break;

			case OPERATOR:
				proxy.SetColor(s, kColorOperator1);
				if (!isOperator(text[i - 1])) {
					i--;
					state = START;
				}
			break;

			default:
				leave = true;
			break;
		}
	}
}
