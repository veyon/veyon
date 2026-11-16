// SPDX-FileCopyrightText: 2026 Tobias Junghans <tobydox@veyon.io>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <QRandomGenerator>
#include <QRegularExpression>
#include <QtTest>

#include "TextMessageSanitizer.h"


class TextMessageSanitizerTest : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void allowlistedMarkupSurvives();
	void allowlistedMarkupSurvives_data();
	void plainTextIsEscaped();
	void qtEditorOutputKeepsFormatting();
	void dangerousTagsAreDropped();
	void dangerousTagsAreDropped_data();
	void commentsAndDeclarationsAreDropped();
	void disallowedAttributesAreDropped();
	void allowedLinksSurvive();
	void allowedLinksSurvive_data();
	void disallowedLinksAreRenderedAsText();
	void disallowedLinksAreRenderedAsText_data();
	void linkTextSurvivesWhenLinkIsRemoved();
	void unsafeStylesAreDropped();
	void malformedMarkupIsBalanced();
	void malformedMarkupIsBalanced_data();
	void nestingDepthIsLimited();
	void oversizeInputIsTruncated();
	void titleIsPlainTextAndLimited();
	void isAllowedLinkChecksSchemes();
	void isAllowedLinkChecksSchemes_data();
	void randomInputIsIdempotentAndSafe();
};



void TextMessageSanitizerTest::allowlistedMarkupSurvives()
{
	QFETCH(QString, input);
	QCOMPARE(TextMessageSanitizer::sanitizeHtml(input), input);
}



void TextMessageSanitizerTest::allowlistedMarkupSurvives_data()
{
	QTest::addColumn<QString>("input");

	QTest::newRow("inline") << QStringLiteral("<b>a</b><i>b</i><u>c</u><s>d</s><code>e</code>");
	QTest::newRow("br") << QStringLiteral("a<br />b");
	QTest::newRow("blocks") << QStringLiteral("<p>a</p><div>b</div><pre>c</pre><blockquote>d</blockquote>");
	QTest::newRow("headings") << QStringLiteral("<h1>a</h1><h2>b</h2><h3>c</h3><h4>d</h4>");
	QTest::newRow("lists") << QStringLiteral("<ul><li>a</li></ul><ol><li>b</li></ol>");
	QTest::newRow("font") << QStringLiteral("<font color=\"#ff0000\" size=\"3\">x</font>");
	QTest::newRow("span") << QStringLiteral("<span>x</span>");
	QTest::newRow("entities") << QStringLiteral("a &lt; b &amp;&amp; c &gt; d &nbsp; &#169;");
}



void TextMessageSanitizerTest::plainTextIsEscaped()
{
	QCOMPARE(TextMessageSanitizer::sanitizeHtml(QStringLiteral("a < b > c & d")),
			 QStringLiteral("a &lt; b &gt; c &amp; d"));
	QCOMPARE(TextMessageSanitizer::sanitizeHtml(QString(QLatin1Char('a')) + QChar(0) + QLatin1Char('b')),
			 QStringLiteral("ab"));
}



void TextMessageSanitizerTest::qtEditorOutputKeepsFormatting()
{
	// typical output of QTextEdit::toHtml()
	const auto input = QStringLiteral(
		"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">"
		"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">p, li { white-space: "
		"pre-wrap; }</style></head><body style=\" font-family:'Sans Serif'; font-size:9pt;\">"
		"<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; -qt-block-indent:0; text-indent:0px;\">"
		"<span style=\" font-weight:600; color:#ff0000;\">Hello</span> "
		"<a href=\"https://example.com/a?b=1&amp;c=2\"><span style=\" text-decoration: underline; "
		"color:#0000ff;\">link</span></a></p></body></html>");

	const auto output = TextMessageSanitizer::sanitizeHtml(input);

	QVERIFY(output.contains(QStringLiteral("<p style=\"margin-top:0px;margin-bottom:0px;text-indent:0px\">")));
	QVERIFY(output.contains(QStringLiteral("align")) == false);
	QVERIFY(output.contains(QStringLiteral("<span style=\"font-weight:600;color:#ff0000\">Hello</span>")));
	QVERIFY(output.contains(QStringLiteral("<a href=\"https://example.com/a?b=1&amp;c=2\">")));
	QVERIFY(output.contains(QStringLiteral("text-decoration:underline;color:#0000ff")));
	QVERIFY(output.contains(QStringLiteral("pre-wrap")) == false);
	QVERIFY(output.contains(QStringLiteral("qrichtext")) == false);
	QVERIFY(output.contains(QStringLiteral("DOCTYPE")) == false);
	QVERIFY(output.contains(QStringLiteral("font-family")) == false);
}



void TextMessageSanitizerTest::dangerousTagsAreDropped()
{
	QFETCH(QString, input);
	QFETCH(QString, expected);
	QCOMPARE(TextMessageSanitizer::sanitizeHtml(input), expected);
}



void TextMessageSanitizerTest::dangerousTagsAreDropped_data()
{
	QTest::addColumn<QString>("input");
	QTest::addColumn<QString>("expected");

	QTest::newRow("img") << QStringLiteral("a<img src=\"http://evil/x.png\">b") << QStringLiteral("ab");
	QTest::newRow("img-unc") << QStringLiteral("<img src=\"\\\\evil\\share\\x.png\"/>") << QString();
	QTest::newRow("img-onerror") << QStringLiteral("<IMG SRC=x onerror=alert(1)>") << QString();
	QTest::newRow("style") << QStringLiteral("a<style>@import url(http://evil/)</style>b") << QStringLiteral("ab");
	QTest::newRow("script") << QStringLiteral("a<script>alert(1)</script>b") << QStringLiteral("ab");
	QTest::newRow("script-case") << QStringLiteral("a<ScRiPt >x</SCRIPT >b") << QStringLiteral("ab");
	QTest::newRow("unterminated-script") << QStringLiteral("a<script>x") << QStringLiteral("a");
	QTest::newRow("link") << QStringLiteral("<link rel=\"stylesheet\" href=\"http://evil/\">x") << QStringLiteral("x");
	QTest::newRow("object") << QStringLiteral("<object data=\"x\">alt</object>y") << QStringLiteral("y");
	QTest::newRow("iframe") << QStringLiteral("<iframe src=\"http://evil/\"></iframe>y") << QStringLiteral("y");
	QTest::newRow("meta") << QStringLiteral("<meta http-equiv=\"refresh\" content=\"0;url=file:///c\">y")
						  << QStringLiteral("y");
	QTest::newRow("table") << QStringLiteral("<table><tr><td>x</td></tr></table>") << QStringLiteral("x");
	QTest::newRow("svg") << QStringLiteral("<svg onload=alert(1)><a>q</a></svg>z") << QStringLiteral("z");
	QTest::newRow("escaped-tag-text") << QStringLiteral("&lt;script&gt;") << QStringLiteral("&lt;script&gt;");
}



void TextMessageSanitizerTest::commentsAndDeclarationsAreDropped()
{
	QCOMPARE(TextMessageSanitizer::sanitizeHtml(QStringLiteral("a<!-- <b>x</b> -->b")), QStringLiteral("ab"));
	QCOMPARE(TextMessageSanitizer::sanitizeHtml(QStringLiteral("a<!-- unterminated <img src=x>")), QStringLiteral("a"));
	QCOMPARE(TextMessageSanitizer::sanitizeHtml(QStringLiteral("a<?xml version=\"1.0\"?>b")), QStringLiteral("ab"));
	QCOMPARE(TextMessageSanitizer::sanitizeHtml(QStringLiteral("a<![CDATA[x]]>b")), QStringLiteral("ab"));
}



void TextMessageSanitizerTest::disallowedAttributesAreDropped()
{
	QCOMPARE(TextMessageSanitizer::sanitizeHtml(QStringLiteral("<b onclick=\"x()\" id=a class=b>t</b>")),
			 QStringLiteral("<b>t</b>"));
	QCOMPARE(TextMessageSanitizer::sanitizeHtml(QStringLiteral("<font face=\"Arial\" color=\"red\" size=\"9\">t</font>")),
			 QStringLiteral("<font color=\"red\">t</font>"));
	QCOMPARE(TextMessageSanitizer::sanitizeHtml(QStringLiteral("<span href=\"http://x/\" color=\"red\">t</span>")),
			 QStringLiteral("<span>t</span>"));
	QCOMPARE(TextMessageSanitizer::sanitizeHtml(QStringLiteral("<a href=\"https://x.org/\" onmouseover=\"y\" target=_blank>t</a>")),
			 QStringLiteral("<a href=\"https://x.org/\">t</a>"));
	QCOMPARE(TextMessageSanitizer::sanitizeHtml(QStringLiteral("<font color=\"red\" color=\"blue\">t</font>")),
			 QStringLiteral("<font color=\"red\">t</font>"));
}



void TextMessageSanitizerTest::allowedLinksSurvive()
{
	QFETCH(QString, input);
	QFETCH(QString, expected);
	QCOMPARE(TextMessageSanitizer::sanitizeHtml(input), expected);
}



void TextMessageSanitizerTest::allowedLinksSurvive_data()
{
	QTest::addColumn<QString>("input");
	QTest::addColumn<QString>("expected");

	QTest::newRow("http") << QStringLiteral("<a href=\"http://example.com/\">x</a>")
						  << QStringLiteral("<a href=\"http://example.com/\">x</a>");
	QTest::newRow("https") << QStringLiteral("<a href=\"https://example.com/p?q=1\">x</a>")
						   << QStringLiteral("<a href=\"https://example.com/p?q=1\">x</a>");
	QTest::newRow("mailto") << QStringLiteral("<a href=\"mailto:teacher@example.com\">x</a>")
							<< QStringLiteral("<a href=\"mailto:teacher@example.com\">x</a>");
	QTest::newRow("uppercase-scheme") << QStringLiteral("<A HREF=\"HTTPS://example.com/\">x</A>")
									  << QStringLiteral("<a href=\"https://example.com/\">x</a>");
	QTest::newRow("single-quotes") << QStringLiteral("<a href='https://example.com/'>x</a>")
								   << QStringLiteral("<a href=\"https://example.com/\">x</a>");
	QTest::newRow("unquoted") << QStringLiteral("<a href=https://example.com/>x</a>")
							  << QStringLiteral("<a href=\"https://example.com/\">x</a>");
	QTest::newRow("surrounding-space") << QStringLiteral("<a href=\"  https://example.com/ \">x</a>")
									   << QStringLiteral("<a href=\"https://example.com/\">x</a>");
	QTest::newRow("encoded-scheme-colon-in-query")
		<< QStringLiteral("<a href=\"https://example.com/?u=file&#58;//x\">x</a>")
		<< QStringLiteral("<a href=\"https://example.com/?u=file://x\">x</a>");
}



void TextMessageSanitizerTest::disallowedLinksAreRenderedAsText()
{
	QFETCH(QString, href);

	const auto output = TextMessageSanitizer::sanitizeHtml(QStringLiteral("<a href=\"%1\">click</a>").arg(href));

	QVERIFY2(output.contains(QStringLiteral("<a")) == false, qPrintable(output));
	QVERIFY2(output.contains(QStringLiteral("href")) == false, qPrintable(output));
	QVERIFY2(output.contains(QStringLiteral("click")), qPrintable(output));
}



void TextMessageSanitizerTest::disallowedLinksAreRenderedAsText_data()
{
	QTest::addColumn<QString>("href");

	QTest::newRow("file") << QStringLiteral("file:///C:/Windows/System32/cmd.exe");
	QTest::newRow("file-upper") << QStringLiteral("FILE:///etc/passwd");
	QTest::newRow("smb") << QStringLiteral("smb://attacker/share");
	QTest::newRow("unc") << QStringLiteral("\\\\attacker\\share");
	QTest::newRow("unc-slashes") << QStringLiteral("//attacker/share");
	QTest::newRow("javascript") << QStringLiteral("javascript:alert(1)");
	QTest::newRow("javascript-case-space") << QStringLiteral(" JaVaScRiPt:alert(1)");
	QTest::newRow("javascript-tab") << QStringLiteral("java&#9;script:alert(1)");
	QTest::newRow("javascript-newline-entity") << QStringLiteral("java&NewLine;script:alert(1)");
	QTest::newRow("javascript-colon-entity") << QStringLiteral("javascript&colon;alert(1)");
	QTest::newRow("javascript-numeric-colon") << QStringLiteral("javascript&#x3a;alert(1)");
	QTest::newRow("javascript-nul") << QStringLiteral("java&#0;script:alert(1)");
	QTest::newRow("data") << QStringLiteral("data:text/html,<b>x</b>");
	QTest::newRow("vbscript") << QStringLiteral("vbscript:msgbox(1)");
	QTest::newRow("custom") << QStringLiteral("ms-msdt:/id PCWDiagnostic");
	QTest::newRow("custom2") << QStringLiteral("search-ms:query=x&crumb=location:\\\\attacker\\share");
	QTest::newRow("ftp") << QStringLiteral("ftp://example.com/");
	QTest::newRow("relative") << QStringLiteral("/etc/passwd");
	QTest::newRow("relative-name") << QStringLiteral("cmd.exe");
	QTest::newRow("empty") << QString();
	QTest::newRow("no-host") << QStringLiteral("http:///x");
	QTest::newRow("http-backslashes") << QStringLiteral("http:\\\\attacker\\share");
	QTest::newRow("embedded-space") << QStringLiteral("https://exa mple.com/");
	QTest::newRow("ctrl-char") << QStringLiteral("https://example.com/&#1;");
	QTest::newRow("empty-mailto") << QStringLiteral("mailto:");
}



void TextMessageSanitizerTest::linkTextSurvivesWhenLinkIsRemoved()
{
	QCOMPARE(TextMessageSanitizer::sanitizeHtml(QStringLiteral("<a href=\"file:///x\"><b>t</b></a>after")),
			 QStringLiteral("<span><b>t</b></span>after"));
	// a stray end tag of a neutralised link must not close an outer valid link
	QCOMPARE(TextMessageSanitizer::sanitizeHtml(QStringLiteral("<a href=\"https://a.org/\">x<a href=\"file:///\">y</a>z</a>")),
			 QStringLiteral("<a href=\"https://a.org/\">x<span>y</span>z</a>"));
	// anchors without href are neutralised as well
	QCOMPARE(TextMessageSanitizer::sanitizeHtml(QStringLiteral("<a name=\"n\">x</a>")), QStringLiteral("<span>x</span>"));
}



void TextMessageSanitizerTest::unsafeStylesAreDropped()
{
	const auto output = TextMessageSanitizer::sanitizeHtml(
		QStringLiteral("<span style=\"color:red; background:url(http://evil/x); font-weight:bold; "
					   "background-color:url(x); font-size:expression(alert(1)); behavior:url(x)\">t</span>"));
	QCOMPARE(output, QStringLiteral("<span style=\"color:red;font-weight:bold\">t</span>"));

	QCOMPARE(TextMessageSanitizer::sanitizeHtml(QStringLiteral("<span style=\"color:red&quot;onclick=&quot;x\">t</span>")),
			 QStringLiteral("<span>t</span>"));
}



void TextMessageSanitizerTest::malformedMarkupIsBalanced()
{
	QFETCH(QString, input);
	QFETCH(QString, expected);
	QCOMPARE(TextMessageSanitizer::sanitizeHtml(input), expected);
}



void TextMessageSanitizerTest::malformedMarkupIsBalanced_data()
{
	QTest::addColumn<QString>("input");
	QTest::addColumn<QString>("expected");

	QTest::newRow("unclosed") << QStringLiteral("<b><i>x") << QStringLiteral("<b><i>x</i></b>");
	QTest::newRow("misnested") << QStringLiteral("<b><i>x</b>y</i>") << QStringLiteral("<b><i>x</i></b>y");
	QTest::newRow("stray-end") << QStringLiteral("x</b></p>y") << QStringLiteral("xy");
	QTest::newRow("unterminated-tag") << QStringLiteral("x<b") << QStringLiteral("x");
	QTest::newRow("unterminated-attribute") << QStringLiteral("x<a href=\"http://a/>y") << QStringLiteral("x");
	QTest::newRow("lt-without-tag") << QStringLiteral("1 <2 and 3< 4") << QStringLiteral("1 &lt;2 and 3&lt; 4");
	QTest::newRow("double-lt") << QStringLiteral("<<b>x</b>") << QStringLiteral("&lt;<b>x</b>");
	QTest::newRow("slash-in-tag") << QStringLiteral("<b/onclick=x>t</b>") << QStringLiteral("<b>t</b>");
	QTest::newRow("empty-tag") << QStringLiteral("<>x</>") << QStringLiteral("&lt;&gt;x");
	QTest::newRow("end-br") << QStringLiteral("a</br>b") << QStringLiteral("ab");
	QTest::newRow("self-closing-b") << QStringLiteral("<b/>x") << QStringLiteral("<b>x</b>");
	QTest::newRow("bare-amp") << QStringLiteral("a&b&amp;c&#x;") << QStringLiteral("a&amp;b&amp;c&amp;#x;");
}



void TextMessageSanitizerTest::nestingDepthIsLimited()
{
	QString input;
	for (int i = 0; i < 1000; ++i)
	{
		input += QStringLiteral("<b>");
	}
	input += QStringLiteral("x");

	const auto output = TextMessageSanitizer::sanitizeHtml(input);
	QCOMPARE(output.count(QStringLiteral("<b>")), TextMessageSanitizer::MaxNestingDepth);
	QCOMPARE(output.count(QStringLiteral("</b>")), TextMessageSanitizer::MaxNestingDepth);
}



void TextMessageSanitizerTest::oversizeInputIsTruncated()
{
	const auto input = QString(TextMessageSanitizer::MaxTextLength * 3, QLatin1Char('a'));
	const auto output = TextMessageSanitizer::sanitizeHtml(input);
	QCOMPARE(output.size(), TextMessageSanitizer::MaxTextLength);

	// truncation in the middle of markup leaves balanced output
	const auto markup = QString(TextMessageSanitizer::MaxTextLength - 2, QLatin1Char('a')) + QStringLiteral("<b>bold</b>");
	const auto markupOutput = TextMessageSanitizer::sanitizeHtml(markup);
	QVERIFY(markupOutput.endsWith(QStringLiteral("aa")));
	QVERIFY(markupOutput.contains(QStringLiteral("<b")) == false);

	const auto markupTail = QString(TextMessageSanitizer::MaxTextLength - 8, QLatin1Char('a')) + QStringLiteral("<b>bold</b>");
	QVERIFY(TextMessageSanitizer::sanitizeHtml(markupTail).endsWith(QStringLiteral("</b>")));
}



void TextMessageSanitizerTest::titleIsPlainTextAndLimited()
{
	QCOMPARE(TextMessageSanitizer::sanitizeTitle(QStringLiteral("Hello")), QStringLiteral("Hello"));
	QCOMPARE(TextMessageSanitizer::sanitizeTitle(QStringLiteral("a\nb\tc\x01"
																"d\x7f")),
			 QStringLiteral("abcd"));
	// the title is never interpreted as markup, it is kept literally
	QCOMPARE(TextMessageSanitizer::sanitizeTitle(QStringLiteral("<b>x</b>")), QStringLiteral("<b>x</b>"));
	QCOMPARE(TextMessageSanitizer::sanitizeTitle(QString(1000, QLatin1Char('x'))).size(),
			 TextMessageSanitizer::MaxTitleLength);
	QVERIFY(TextMessageSanitizer::sanitizeTitle({}).isEmpty());
}



void TextMessageSanitizerTest::isAllowedLinkChecksSchemes()
{
	QFETCH(QString, link);
	QFETCH(bool, allowed);
	QCOMPARE(TextMessageSanitizer::isAllowedLink(link), allowed);
	QCOMPARE(TextMessageSanitizer::isAllowedLink(QUrl(link, QUrl::StrictMode)), allowed);
}



void TextMessageSanitizerTest::isAllowedLinkChecksSchemes_data()
{
	QTest::addColumn<QString>("link");
	QTest::addColumn<bool>("allowed");

	QTest::newRow("http") << QStringLiteral("http://example.com") << true;
	QTest::newRow("https") << QStringLiteral("HTTPS://example.com/x") << true;
	QTest::newRow("mailto") << QStringLiteral("mailto:a@example.com") << true;
	QTest::newRow("file") << QStringLiteral("file:///etc/passwd") << false;
	QTest::newRow("smb") << QStringLiteral("smb://h/s") << false;
	QTest::newRow("unc") << QStringLiteral("\\\\h\\s") << false;
	QTest::newRow("javascript") << QStringLiteral("javascript:alert(1)") << false;
	QTest::newRow("empty") << QString() << false;
	QTest::newRow("no-host") << QStringLiteral("https://") << false;
}



void TextMessageSanitizerTest::randomInputIsIdempotentAndSafe()
{
	static const QStringList fragments{
		QStringLiteral("<"),
		QStringLiteral(">"),
		QStringLiteral("</"),
		QStringLiteral("/>"),
		QStringLiteral("<a href=\""),
		QStringLiteral("\""),
		QStringLiteral("'"),
		QStringLiteral("="),
		QStringLiteral("&"),
		QStringLiteral("&#58;"),
		QStringLiteral("b"),
		QStringLiteral("i"),
		QStringLiteral("script"),
		QStringLiteral("style"),
		QStringLiteral("img"),
		QStringLiteral("<font color="),
		QStringLiteral(" "),
		QStringLiteral("javascript:"),
		QStringLiteral("file://"),
		QStringLiteral("https://x/"),
		QStringLiteral("<!--"),
		QStringLiteral("-->"),
		QStringLiteral("<span style=\""),
		QStringLiteral(";"),
		QStringLiteral(":"),
		QStringLiteral("color"),
		QStringLiteral("red"),
		QStringLiteral("\n"),
		QStringLiteral("<br>"),
		QStringLiteral("<b>"),
		QStringLiteral("</b>"),
	};

	QRandomGenerator generator(12345);
	for (int run = 0; run < 20000; ++run)
	{
		QString input;
		const auto count = generator.bounded(1, 30);
		for (int i = 0; i < count; ++i)
		{
			input += fragments.at(static_cast<int>(generator.bounded(static_cast<quint32>(fragments.size()))));
		}

		const auto output = TextMessageSanitizer::sanitizeHtml(input);
		QVERIFY2(TextMessageSanitizer::sanitizeHtml(output) == output, qPrintable(input + QStringLiteral(" -> ") + output));

		// markup of disallowed tags must never appear, links must carry allowed schemes only
		const auto lowered = output.toLower();
		QVERIFY2(lowered.contains(QStringLiteral("<script")) == false && lowered.contains(QStringLiteral("<img")) == false &&
					 lowered.contains(QStringLiteral("<style")) == false,
				 qPrintable(input + QStringLiteral(" -> ") + output));

		auto hrefs = QRegularExpression(QStringLiteral("href=\"([^\"]*)\"")).globalMatch(lowered);
		while (hrefs.hasNext())
		{
			const auto href = hrefs.next().captured(1);
			QVERIFY2(href.startsWith(QStringLiteral("http://")) || href.startsWith(QStringLiteral("https://")) ||
						 href.startsWith(QStringLiteral("mailto:")),
					 qPrintable(input + QStringLiteral(" -> ") + output));
		}
	}
}


QTEST_APPLESS_MAIN(TextMessageSanitizerTest)

#include "TextMessageSanitizerTest.moc"
