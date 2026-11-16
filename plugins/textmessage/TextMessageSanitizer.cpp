// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QList>
#include <QPair>
#include <QRegularExpression>
#include <QSet>
#include <QStringList>

#include "TextMessageSanitizer.h"


namespace {

struct OpenTag
{
	QString sourceName;
	QString emittedName;
};

struct ParsedTag
{
	QString name;
	bool closing = false;
	QList<QPair<QString, QString>> attributes;
	int end = 0; // position after the closing '>'
};


const QSet<QString>& allowedTags()
{
	static const QSet<QString> tags{
		QStringLiteral("a"),
		QStringLiteral("b"),
		QStringLiteral("i"),
		QStringLiteral("u"),
		QStringLiteral("s"),
		QStringLiteral("br"),
		QStringLiteral("p"),
		QStringLiteral("div"),
		QStringLiteral("span"),
		QStringLiteral("ul"),
		QStringLiteral("ol"),
		QStringLiteral("li"),
		QStringLiteral("h1"),
		QStringLiteral("h2"),
		QStringLiteral("h3"),
		QStringLiteral("h4"),
		QStringLiteral("font"),
		QStringLiteral("code"),
		QStringLiteral("pre"),
		QStringLiteral("blockquote"),
	};
	return tags;
}



// tags whose content must not be shown at all (not just the tag itself)
const QSet<QString>& skippedContentTags()
{
	static const QSet<QString> tags{
		QStringLiteral("script"),
		QStringLiteral("style"),
		QStringLiteral("title"),
		QStringLiteral("head"),
		QStringLiteral("noscript"),
		QStringLiteral("template"),
		QStringLiteral("object"),
		QStringLiteral("iframe"),
		QStringLiteral("embed"),
		QStringLiteral("applet"),
		QStringLiteral("svg"),
		QStringLiteral("math"),
		QStringLiteral("textarea"),
		QStringLiteral("xmp"),
		QStringLiteral("noembed"),
		QStringLiteral("noframes"),
		QStringLiteral("plaintext"),
	};
	return tags;
}



bool isAsciiLetter(QChar c)
{
	return (c >= QLatin1Char('a') && c <= QLatin1Char('z')) || (c >= QLatin1Char('A') && c <= QLatin1Char('Z'));
}



bool isHtmlSpace(QChar c)
{
	return c == QLatin1Char(' ') || c == QLatin1Char('\t') || c == QLatin1Char('\n') || c == QLatin1Char('\r') ||
		c == QLatin1Char('\f');
}



bool isForbiddenControlChar(QChar c)
{
	const auto code = c.unicode();
	return code == 0 || (code < 0x20 && c != QLatin1Char('\t') && c != QLatin1Char('\n') && c != QLatin1Char('\r')) ||
		code == 0x7f;
}



// Parses a tag starting at '<' (input[start]). Returns false if the tag is unterminated.
bool parseTag(const QString& input, int start, ParsedTag& tag)
{
	const auto size = input.size();
	int pos = start + 1;

	if (pos < size && input[pos] == QLatin1Char('/'))
	{
		tag.closing = true;
		++pos;
	}

	const auto nameStart = pos;
	while (pos < size && isHtmlSpace(input[pos]) == false && input[pos] != QLatin1Char('/') &&
		   input[pos] != QLatin1Char('>'))
	{
		++pos;
	}
	tag.name = input.mid(nameStart, pos - nameStart).toLower();

	while (true)
	{
		while (pos < size && (isHtmlSpace(input[pos]) || input[pos] == QLatin1Char('/')))
		{
			++pos;
		}

		if (pos >= size)
		{
			return false;
		}

		if (input[pos] == QLatin1Char('>'))
		{
			tag.end = pos + 1;
			return true;
		}

		const auto attributeStart = pos;
		while (pos < size && isHtmlSpace(input[pos]) == false && input[pos] != QLatin1Char('=') &&
			   input[pos] != QLatin1Char('/') && input[pos] != QLatin1Char('>'))
		{
			++pos;
		}
		const auto attributeName = input.mid(attributeStart, pos - attributeStart).toLower();

		while (pos < size && isHtmlSpace(input[pos]))
		{
			++pos;
		}

		QString value;
		if (pos < size && input[pos] == QLatin1Char('='))
		{
			++pos;
			while (pos < size && isHtmlSpace(input[pos]))
			{
				++pos;
			}
			if (pos >= size)
			{
				return false;
			}

			if (input[pos] == QLatin1Char('"') || input[pos] == QLatin1Char('\''))
			{
				const auto quote = input[pos];
				const auto valueEnd = input.indexOf(quote, pos + 1);
				if (valueEnd < 0)
				{
					return false;
				}
				value = input.mid(pos + 1, valueEnd - pos - 1);
				pos = valueEnd + 1;
			}
			else
			{
				const auto valueStart = pos;
				while (pos < size && isHtmlSpace(input[pos]) == false && input[pos] != QLatin1Char('>'))
				{
					++pos;
				}
				value = input.mid(valueStart, pos - valueStart);
			}
		}

		if (attributeName.isEmpty() == false && tag.attributes.size() < 32)
		{
			tag.attributes.append({attributeName, value});
		}
	}
}



// decodes numeric and a few named character references; everything else is kept literally
QString decodeCharacterReferences(const QString& value)
{
	static const QRegularExpression referenceRegex(QStringLiteral("&(#[0-9]{1,7}|#[xX][0-9a-fA-F]{1,6}|[a-zA-Z]{2,8});?"));

	QString result;
	result.reserve(value.size());

	int pos = 0;
	auto it = referenceRegex.globalMatch(value);
	while (it.hasNext())
	{
		const auto match = it.next();
		result += value.mid(pos, match.capturedStart() - pos);
		pos = match.capturedEnd();

		const auto reference = match.captured(1);
		QString decoded;
		if (reference.startsWith(QLatin1Char('#')))
		{
			bool ok = false;
			const auto code = reference.at(1).toLower() == QLatin1Char('x') ? reference.mid(2).toUInt(&ok, 16) : reference.mid(1).toUInt(&ok, 10);
			if (ok && code > 0 && code <= 0x10ffff && (code < 0xd800 || code > 0xdfff))
			{
				decoded = QString::fromUcs4(reinterpret_cast<const char32_t*>(&code), 1);
			}
		}
		else
		{
			static const QList<QPair<QString, QString>> named{
				{QStringLiteral("amp"), QStringLiteral("&")},
				{QStringLiteral("lt"), QStringLiteral("<")},
				{QStringLiteral("gt"), QStringLiteral(">")},
				{QStringLiteral("quot"), QStringLiteral("\"")},
				{QStringLiteral("apos"), QStringLiteral("'")},
				{QStringLiteral("colon"), QStringLiteral(":")},
				{QStringLiteral("Tab"), QStringLiteral("\t")},
				{QStringLiteral("NewLine"), QStringLiteral("\n")},
				{QStringLiteral("sol"), QStringLiteral("/")},
				{QStringLiteral("bsol"), QStringLiteral("\\")},
				{QStringLiteral("nbsp"), QStringLiteral("\xa0")},
			};
			for (const auto& entry : named)
			{
				if (entry.first == reference)
				{
					decoded = entry.second;
					break;
				}
			}
		}

		// unknown references are kept literally so that they fail validation later
		result += decoded.isEmpty() ? match.captured(0) : decoded;
	}

	result += value.mid(pos);

	return result;
}



QString escapeAttribute(const QString& value)
{
	QString result;
	result.reserve(value.size());
	for (const auto c : value)
	{
		switch (c.unicode())
		{
		case '&': result += QLatin1String("&amp;"); break;
		case '"': result += QLatin1String("&quot;"); break;
		case '<': result += QLatin1String("&lt;"); break;
		case '>': result += QLatin1String("&gt;"); break;
		default: result += c; break;
		}
	}
	return result;
}



// escapes text so that the output never contains markup of its own
void appendEscapedText(QString& output, const QString& text)
{
	static const QRegularExpression entityRegex(
		QStringLiteral("^&(#[0-9]{1,7}|#[xX][0-9a-fA-F]{1,6}|[a-zA-Z][a-zA-Z0-9]{1,31});"));

	for (int i = 0; i < text.size(); ++i)
	{
		const auto c = text[i];
		if (c == QLatin1Char('<'))
		{
			output += QLatin1String("&lt;");
		}
		else if (c == QLatin1Char('>'))
		{
			output += QLatin1String("&gt;");
		}
		else if (c == QLatin1Char('&'))
		{
			const auto match = entityRegex.match(text.mid(i, 40));
			if (match.hasMatch())
			{
				output += match.captured(0);
				i += match.capturedLength() - 1;
			}
			else
			{
				output += QLatin1String("&amp;");
			}
		}
		else if (isForbiddenControlChar(c) == false)
		{
			output += c;
		}
	}
}



QString sanitizeStyle(const QString& style)
{
	struct Property
	{
		QString name;
		QRegularExpression valueRegex;
	};

	static const auto colorPattern = QStringLiteral("^(#[0-9a-f]{3}|#[0-9a-f]{6}|[a-z]{3,20})$");
	static const auto lengthPattern = QStringLiteral("^[0-9]{1,3}(\\.[0-9]{1,2})?(px|pt)?$");
	static const auto decorationWord = QStringLiteral("(underline|line-through|overline)");

	static const QList<Property> properties{
		{QStringLiteral("color"), QRegularExpression(colorPattern, QRegularExpression::CaseInsensitiveOption)},
		{QStringLiteral("background-color"),
		 QRegularExpression(colorPattern, QRegularExpression::CaseInsensitiveOption)},
		{QStringLiteral("font-size"), QRegularExpression(QStringLiteral("^[0-9]{1,3}(\\.[0-9]{1,2})?(px|pt)$"))},
		{QStringLiteral("font-weight"), QRegularExpression(QStringLiteral("^(normal|bold|[1-9]00)$"))},
		{QStringLiteral("font-style"), QRegularExpression(QStringLiteral("^(normal|italic|oblique)$"))},
		{QStringLiteral("text-decoration"),
		 QRegularExpression(QStringLiteral("^(none|%1( %1)*)$").arg(decorationWord))},
		{QStringLiteral("margin-top"), QRegularExpression(lengthPattern)},
		{QStringLiteral("margin-bottom"), QRegularExpression(lengthPattern)},
		{QStringLiteral("margin-left"), QRegularExpression(lengthPattern)},
		{QStringLiteral("margin-right"), QRegularExpression(lengthPattern)},
		{QStringLiteral("text-indent"), QRegularExpression(lengthPattern)},
	};

	QStringList result;
	const auto declarations = decodeCharacterReferences(style).split(QLatin1Char(';'));
	for (const auto& declaration : declarations)
	{
		const auto separator = declaration.indexOf(QLatin1Char(':'));
		if (separator < 0)
		{
			continue;
		}

		const auto name = declaration.left(separator).trimmed().toLower();
		auto value = declaration.mid(separator + 1).trimmed();
		value.replace(QRegularExpression(QStringLiteral("\\s+")), QStringLiteral(" "));

		for (const auto& property : properties)
		{
			if (property.name == name && property.valueRegex.match(value).hasMatch())
			{
				result.append(name + QLatin1Char(':') + value.toLower());
				break;
			}
		}
	}

	return result.join(QLatin1Char(';'));
}



// returns the canonical, safe form of an href value or an empty string
QString sanitizeLink(const QString& rawValue)
{
	auto value = decodeCharacterReferences(rawValue).trimmed();
	for (const auto c : value)
	{
		if (c.unicode() < 0x20 || c.unicode() == 0x7f)
		{
			return {};
		}
	}

	const QUrl url(value, QUrl::StrictMode);
	if (TextMessageSanitizer::isAllowedLink(url) == false)
	{
		return {};
	}

	return QString::fromLatin1(url.toEncoded());
}



QString buildAttributes(const ParsedTag& tag, const QString& emittedName)
{
	QString result;
	QSet<QString> seen;

	for (const auto& attribute : tag.attributes)
	{
		const auto& name = attribute.first;
		if (seen.contains(name))
		{
			continue;
		}

		QString value;
		if (name == QLatin1String("style"))
		{
			value = sanitizeStyle(attribute.second);
		}
		else if (emittedName == QLatin1String("a") && name == QLatin1String("href"))
		{
			value = sanitizeLink(attribute.second);
		}
		else if (emittedName == QLatin1String("font") && name == QLatin1String("color"))
		{
			static const QRegularExpression colorRegex(QStringLiteral("^(#[0-9a-f]{3}|#[0-9a-f]{6}|[a-z]{3,20})$"),
													   QRegularExpression::CaseInsensitiveOption);
			value = decodeCharacterReferences(attribute.second).trimmed();
			if (colorRegex.match(value).hasMatch() == false)
			{
				value.clear();
			}
		}
		else if (emittedName == QLatin1String("font") && name == QLatin1String("size"))
		{
			static const QRegularExpression sizeRegex(QStringLiteral("^[+-]?[1-7]$"));
			value = decodeCharacterReferences(attribute.second).trimmed();
			if (sizeRegex.match(value).hasMatch() == false)
			{
				value.clear();
			}
		}

		seen.insert(name);

		if (value.isEmpty() == false)
		{
			result += QLatin1Char(' ') + name + QLatin1String("=\"") + escapeAttribute(value) + QLatin1Char('"');
		}
	}

	return result;
}

} // namespace



QString TextMessageSanitizer::sanitizeHtml(const QString& html)
{
	const auto input = html.left(MaxTextLength);
	const auto size = input.size();

	QString output;
	output.reserve(size);

	QList<OpenTag> openTags;

	int pos = 0;
	while (pos < size)
	{
		if (input[pos] != QLatin1Char('<'))
		{
			auto textEnd = input.indexOf(QLatin1Char('<'), pos);
			if (textEnd < 0)
			{
				textEnd = size;
			}
			appendEscapedText(output, input.mid(pos, textEnd - pos));
			pos = textEnd;
			continue;
		}

		const auto next = pos + 1 < size ? input[pos + 1] : QChar();

		if (input.mid(pos, 4) == QLatin1String("<!--"))
		{
			const auto commentEnd = input.indexOf(QLatin1String("-->"), pos + 4);
			pos = commentEnd < 0 ? size : commentEnd + 3;
			continue;
		}

		if (next == QLatin1Char('!') || next == QLatin1Char('?'))
		{
			const auto declarationEnd = input.indexOf(QLatin1Char('>'), pos);
			pos = declarationEnd < 0 ? size : declarationEnd + 1;
			continue;
		}

		if (isAsciiLetter(next) == false && next != QLatin1Char('/'))
		{
			output += QLatin1String("&lt;");
			++pos;
			continue;
		}

		ParsedTag tag;
		if (parseTag(input, pos, tag) == false)
		{
			// unterminated tag: drop the remainder
			break;
		}
		pos = tag.end;

		if (tag.closing)
		{
			for (int i = openTags.size() - 1; i >= 0; --i)
			{
				if (openTags[i].sourceName == tag.name)
				{
					while (openTags.size() > i)
					{
						output += QLatin1String("</") + openTags.takeLast().emittedName + QLatin1Char('>');
					}
					break;
				}
			}
			continue;
		}

		if (skippedContentTags().contains(tag.name))
		{
			const auto endTagPos = input.indexOf(QLatin1String("</") + tag.name, pos, Qt::CaseInsensitive);
			if (endTagPos < 0)
			{
				break;
			}
			const auto endTagEnd = input.indexOf(QLatin1Char('>'), endTagPos);
			pos = endTagEnd < 0 ? size : endTagEnd + 1;
			continue;
		}

		if (allowedTags().contains(tag.name) == false)
		{
			continue;
		}

		if (tag.name == QLatin1String("br"))
		{
			output += QLatin1String("<br />");
			continue;
		}

		if (openTags.size() >= MaxNestingDepth)
		{
			continue;
		}

		auto emittedName = tag.name;
		if (tag.name == QLatin1String("a"))
		{
			bool linkAllowed = false;
			for (const auto& attribute : tag.attributes)
			{
				if (attribute.first == QLatin1String("href"))
				{
					linkAllowed = sanitizeLink(attribute.second).isEmpty() == false;
					break;
				}
			}
			if (linkAllowed == false)
			{
				// keep the text but neutralise the link
				emittedName = QStringLiteral("span");
			}
		}

		output += QLatin1Char('<') + emittedName + buildAttributes(tag, emittedName) + QLatin1Char('>');
		openTags.append({tag.name, emittedName});
	}

	while (openTags.isEmpty() == false)
	{
		output += QLatin1String("</") + openTags.takeLast().emittedName + QLatin1Char('>');
	}

	return output;
}



QString TextMessageSanitizer::sanitizeTitle(const QString& title)
{
	QString result;
	result.reserve(qMin(title.size(), MaxTitleLength));

	for (const auto c : title)
	{
		if (result.size() >= MaxTitleLength)
		{
			break;
		}
		if (c.unicode() < 0x20 || c.unicode() == 0x7f || c.category() == QChar::Other_Control)
		{
			continue;
		}
		result += c;
	}

	return result;
}



bool TextMessageSanitizer::isAllowedLink(const QUrl& url)
{
	if (url.isValid() == false || url.isRelative())
	{
		return false;
	}

	const auto scheme = url.scheme().toLower();

	if (scheme == QLatin1String("http") || scheme == QLatin1String("https"))
	{
		return url.host().isEmpty() == false;
	}

	if (scheme == QLatin1String("mailto"))
	{
		return url.path().isEmpty() == false;
	}

	return false;
}



bool TextMessageSanitizer::isAllowedLink(const QString& link)
{
	return isAllowedLink(QUrl(link.trimmed(), QUrl::StrictMode));
}
