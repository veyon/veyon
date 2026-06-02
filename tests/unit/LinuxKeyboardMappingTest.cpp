/*
 * LinuxKeyboardMappingTest.cpp - test keysym/UTF-8 to Linux keycode mapping
 *
 * Copyright (c) 2019-2026 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include <QObject>
#include <QTest>

// include linux input constants for KEY_MENU etc.
#include <linux/input.h>


#define private public
#include "LinuxKeyboardInput.h"
#undef private

class LinuxKeyboardMappingTest : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	void testKeysymToLinuxKeycode_data();
	void testKeysymToLinuxKeycode();
	void testUtf8ToLinuxKeycode_data();
	void testUtf8ToLinuxKeycode();
	void testUnmappedKeysymReturnsMinusOne();
	void testUnmappedUtf8ReturnsMinusOne();
};

void LinuxKeyboardMappingTest::testKeysymToLinuxKeycode_data()
{
	QTest::addColumn<quint32>("keysym");
	QTest::addColumn<int>("expectedKeycode");

	// a-z
	QTest::addRow("a") << quint32(0x0061) << 30;
	QTest::addRow("b") << quint32(0x0062) << 48;
	QTest::addRow("c") << quint32(0x0063) << 46;
	QTest::addRow("d") << quint32(0x0064) << 32;
	QTest::addRow("e") << quint32(0x0065) << 18;
	QTest::addRow("f") << quint32(0x0066) << 33;
	QTest::addRow("g") << quint32(0x0067) << 34;
	QTest::addRow("h") << quint32(0x0068) << 35;
	QTest::addRow("i") << quint32(0x0069) << 23;
	QTest::addRow("j") << quint32(0x006a) << 36;
	QTest::addRow("k") << quint32(0x006b) << 37;
	QTest::addRow("l") << quint32(0x006c) << 38;
	QTest::addRow("m") << quint32(0x006d) << 50;
	QTest::addRow("n") << quint32(0x006e) << 49;
	QTest::addRow("o") << quint32(0x006f) << 24;
	QTest::addRow("p") << quint32(0x0070) << 25;
	QTest::addRow("q") << quint32(0x0071) << 16;
	QTest::addRow("r") << quint32(0x0072) << 19;
	QTest::addRow("s") << quint32(0x0073) << 31;
	QTest::addRow("t") << quint32(0x0074) << 20;
	QTest::addRow("u") << quint32(0x0075) << 22;
	QTest::addRow("v") << quint32(0x0076) << 47;
	QTest::addRow("w") << quint32(0x0077) << 17;
	QTest::addRow("x") << quint32(0x0078) << 45;
	QTest::addRow("y") << quint32(0x0079) << 21;
	QTest::addRow("z") << quint32(0x007a) << 44;

	// A-Z
	QTest::addRow("A") << quint32(0x0041) << 30;
	QTest::addRow("B") << quint32(0x0042) << 48;
	QTest::addRow("C") << quint32(0x0043) << 46;
	QTest::addRow("D") << quint32(0x0044) << 32;
	QTest::addRow("E") << quint32(0x0045) << 18;
	QTest::addRow("F") << quint32(0x0046) << 33;
	QTest::addRow("G") << quint32(0x0047) << 34;
	QTest::addRow("H") << quint32(0x0048) << 35;
	QTest::addRow("I") << quint32(0x0049) << 23;
	QTest::addRow("J") << quint32(0x004a) << 36;
	QTest::addRow("K") << quint32(0x004b) << 37;
	QTest::addRow("L") << quint32(0x004c) << 38;
	QTest::addRow("M") << quint32(0x004d) << 50;
	QTest::addRow("N") << quint32(0x004e) << 49;
	QTest::addRow("O") << quint32(0x004f) << 24;
	QTest::addRow("P") << quint32(0x0050) << 25;
	QTest::addRow("Q") << quint32(0x0051) << 16;
	QTest::addRow("R") << quint32(0x0052) << 19;
	QTest::addRow("S") << quint32(0x0053) << 31;
	QTest::addRow("T") << quint32(0x0054) << 20;
	QTest::addRow("U") << quint32(0x0055) << 22;
	QTest::addRow("V") << quint32(0x0056) << 47;
	QTest::addRow("W") << quint32(0x0057) << 17;
	QTest::addRow("X") << quint32(0x0058) << 45;
	QTest::addRow("Y") << quint32(0x0059) << 21;
	QTest::addRow("Z") << quint32(0x005a) << 44;

	// Digits 0-9
	QTest::addRow("digit_0") << quint32(0x0030) << 11;
	QTest::addRow("digit_1") << quint32(0x0031) << 2;
	QTest::addRow("digit_2") << quint32(0x0032) << 3;
	QTest::addRow("digit_3") << quint32(0x0033) << 4;
	QTest::addRow("digit_4") << quint32(0x0034) << 5;
	QTest::addRow("digit_5") << quint32(0x0035) << 6;
	QTest::addRow("digit_6") << quint32(0x0036) << 7;
	QTest::addRow("digit_7") << quint32(0x0037) << 8;
	QTest::addRow("digit_8") << quint32(0x0038) << 9;
	QTest::addRow("digit_9") << quint32(0x0039) << 10;

	// Punctuation and symbols
	QTest::addRow("Minus") << quint32(0x002d) << 12;
	QTest::addRow("Equal") << quint32(0x003d) << 13;
	QTest::addRow("BracketLeft") << quint32(0x005b) << 26;
	QTest::addRow("BracketRight") << quint32(0x005d) << 27;
	QTest::addRow("Semicolon") << quint32(0x003b) << 39;
	QTest::addRow("Apostrophe") << quint32(0x0027) << 40;
	QTest::addRow("Grave") << quint32(0x0060) << 41;
	QTest::addRow("Backslash") << quint32(0x005c) << 43;
	QTest::addRow("Comma") << quint32(0x002c) << 51;
	QTest::addRow("Period") << quint32(0x002e) << 52;
	QTest::addRow("Slash") << quint32(0x002f) << 53;

	// Modifier keys
	QTest::addRow("Shift_L") << quint32(0xffe1) << 42;
	QTest::addRow("Shift_R") << quint32(0xffe2) << 54;
	QTest::addRow("Control_L") << quint32(0xffe3) << 29;
	QTest::addRow("Control_R") << quint32(0xffe4) << 97;
	QTest::addRow("Alt_L") << quint32(0xffe9) << 56;
	QTest::addRow("Alt_R") << quint32(0xffea) << 100;
	QTest::addRow("Super_L") << quint32(0xffeb) << 125;
	QTest::addRow("Super_R") << quint32(0xffec) << 126;
	QTest::addRow("Menu") << quint32(0xff67) << KEY_MENU;

	// Whitespace and editing keys
	QTest::addRow("Space") << quint32(0x0020) << 57;
	QTest::addRow("Return") << quint32(0xff0d) << 28;
	QTest::addRow("Tab") << quint32(0xff09) << 15;
	QTest::addRow("BackSpace") << quint32(0xff08) << 14;
	QTest::addRow("Escape") << quint32(0xff1b) << 1;

	// Navigation keys
	QTest::addRow("Home") << quint32(0xff50) << 102;
	QTest::addRow("Up") << quint32(0xff52) << 103;
	QTest::addRow("Prior") << quint32(0xff55) << 104;
	QTest::addRow("Left") << quint32(0xff51) << 105;
	QTest::addRow("Right") << quint32(0xff53) << 106;
	QTest::addRow("End") << quint32(0xff57) << 107;
	QTest::addRow("Down") << quint32(0xff54) << 108;
	QTest::addRow("Next") << quint32(0xff56) << 109;
	QTest::addRow("Insert") << quint32(0xff63) << 110;
	QTest::addRow("Delete") << quint32(0xffff) << 111;

	// Function keys
	QTest::addRow("F1") << quint32(0xffbe) << 59;
	QTest::addRow("F2") << quint32(0xffbf) << 60;
	QTest::addRow("F3") << quint32(0xffc0) << 61;
	QTest::addRow("F4") << quint32(0xffc1) << 62;
	QTest::addRow("F5") << quint32(0xffc2) << 63;
	QTest::addRow("F6") << quint32(0xffc3) << 64;
	QTest::addRow("F7") << quint32(0xffc4) << 65;
	QTest::addRow("F8") << quint32(0xffc5) << 66;
	QTest::addRow("F9") << quint32(0xffc6) << 67;
	QTest::addRow("F10") << quint32(0xffc7) << 68;
	QTest::addRow("F11") << quint32(0xffc8) << 87;
	QTest::addRow("F12") << quint32(0xffc9) << 88;

	// Lock keys
	QTest::addRow("Caps_Lock") << quint32(0xffe5) << 58;
	QTest::addRow("Num_Lock") << quint32(0xff7f) << 69;
	QTest::addRow("Scroll_Lock") << quint32(0xff14) << 70;
	QTest::addRow("Print") << quint32(0xff61) << 99;

	// Keypad keys
	QTest::addRow("KP_Multiply") << quint32(0xffaa) << 55;
	QTest::addRow("KP_7") << quint32(0xffb7) << 71;
	QTest::addRow("KP_8") << quint32(0xffb8) << 72;
	QTest::addRow("KP_9") << quint32(0xffb9) << 73;
	QTest::addRow("KP_Subtract") << quint32(0xffad) << 74;
	QTest::addRow("KP_4") << quint32(0xffb4) << 75;
	QTest::addRow("KP_5") << quint32(0xffb5) << 76;
	QTest::addRow("KP_6") << quint32(0xffb6) << 77;
	QTest::addRow("KP_Add") << quint32(0xffab) << 78;
	QTest::addRow("KP_1") << quint32(0xffb1) << 79;
	QTest::addRow("KP_2") << quint32(0xffb2) << 80;
	QTest::addRow("KP_3") << quint32(0xffb3) << 81;
	QTest::addRow("KP_0") << quint32(0xffb0) << 82;
	QTest::addRow("KP_Decimal") << quint32(0xffae) << 83;
	QTest::addRow("KP_Enter") << quint32(0xff8d) << 96;
	QTest::addRow("KP_Divide") << quint32(0xffaf) << 98;
}

void LinuxKeyboardMappingTest::testKeysymToLinuxKeycode()
{
	QFETCH(quint32, keysym);
	QFETCH(int, expectedKeycode);

	QCOMPARE(LinuxKeyboardInput::keysymToLinuxKeycode(keysym), expectedKeycode);
}

void LinuxKeyboardMappingTest::testUtf8ToLinuxKeycode_data()
{
	QTest::addColumn<QByteArray>("utf8");
	QTest::addColumn<int>("expectedKeycode");

	// a-z
	QTest::addRow("a") << QByteArrayLiteral("a") << 30;
	QTest::addRow("b") << QByteArrayLiteral("b") << 48;
	QTest::addRow("c") << QByteArrayLiteral("c") << 46;
	QTest::addRow("d") << QByteArrayLiteral("d") << 32;
	QTest::addRow("e") << QByteArrayLiteral("e") << 18;
	QTest::addRow("f") << QByteArrayLiteral("f") << 33;
	QTest::addRow("g") << QByteArrayLiteral("g") << 34;
	QTest::addRow("h") << QByteArrayLiteral("h") << 35;
	QTest::addRow("i") << QByteArrayLiteral("i") << 23;
	QTest::addRow("j") << QByteArrayLiteral("j") << 36;
	QTest::addRow("k") << QByteArrayLiteral("k") << 37;
	QTest::addRow("l") << QByteArrayLiteral("l") << 38;
	QTest::addRow("m") << QByteArrayLiteral("m") << 50;
	QTest::addRow("n") << QByteArrayLiteral("n") << 49;
	QTest::addRow("o") << QByteArrayLiteral("o") << 24;
	QTest::addRow("p") << QByteArrayLiteral("p") << 25;
	QTest::addRow("q") << QByteArrayLiteral("q") << 16;
	QTest::addRow("r") << QByteArrayLiteral("r") << 19;
	QTest::addRow("s") << QByteArrayLiteral("s") << 31;
	QTest::addRow("t") << QByteArrayLiteral("t") << 20;
	QTest::addRow("u") << QByteArrayLiteral("u") << 22;
	QTest::addRow("v") << QByteArrayLiteral("v") << 47;
	QTest::addRow("w") << QByteArrayLiteral("w") << 17;
	QTest::addRow("x") << QByteArrayLiteral("x") << 45;
	QTest::addRow("y") << QByteArrayLiteral("y") << 21;
	QTest::addRow("z") << QByteArrayLiteral("z") << 44;

	// A-Z
	QTest::addRow("A") << QByteArrayLiteral("A") << 30;
	QTest::addRow("B") << QByteArrayLiteral("B") << 48;
	QTest::addRow("C") << QByteArrayLiteral("C") << 46;
	QTest::addRow("D") << QByteArrayLiteral("D") << 32;
	QTest::addRow("E") << QByteArrayLiteral("E") << 18;
	QTest::addRow("F") << QByteArrayLiteral("F") << 33;
	QTest::addRow("G") << QByteArrayLiteral("G") << 34;
	QTest::addRow("H") << QByteArrayLiteral("H") << 35;
	QTest::addRow("I") << QByteArrayLiteral("I") << 23;
	QTest::addRow("J") << QByteArrayLiteral("J") << 36;
	QTest::addRow("K") << QByteArrayLiteral("K") << 37;
	QTest::addRow("L") << QByteArrayLiteral("L") << 38;
	QTest::addRow("M") << QByteArrayLiteral("M") << 50;
	QTest::addRow("N") << QByteArrayLiteral("N") << 49;
	QTest::addRow("O") << QByteArrayLiteral("O") << 24;
	QTest::addRow("P") << QByteArrayLiteral("P") << 25;
	QTest::addRow("Q") << QByteArrayLiteral("Q") << 16;
	QTest::addRow("R") << QByteArrayLiteral("R") << 19;
	QTest::addRow("S") << QByteArrayLiteral("S") << 31;
	QTest::addRow("T") << QByteArrayLiteral("T") << 20;
	QTest::addRow("U") << QByteArrayLiteral("U") << 22;
	QTest::addRow("V") << QByteArrayLiteral("V") << 47;
	QTest::addRow("W") << QByteArrayLiteral("W") << 17;
	QTest::addRow("X") << QByteArrayLiteral("X") << 45;
	QTest::addRow("Y") << QByteArrayLiteral("Y") << 21;
	QTest::addRow("Z") << QByteArrayLiteral("Z") << 44;

	// Digits
	QTest::addRow("digit_0") << QByteArrayLiteral("0") << 11;
	QTest::addRow("digit_1") << QByteArrayLiteral("1") << 2;
	QTest::addRow("digit_2") << QByteArrayLiteral("2") << 3;
	QTest::addRow("digit_3") << QByteArrayLiteral("3") << 4;
	QTest::addRow("digit_4") << QByteArrayLiteral("4") << 5;
	QTest::addRow("digit_5") << QByteArrayLiteral("5") << 6;
	QTest::addRow("digit_6") << QByteArrayLiteral("6") << 7;
	QTest::addRow("digit_7") << QByteArrayLiteral("7") << 8;
	QTest::addRow("digit_8") << QByteArrayLiteral("8") << 9;
	QTest::addRow("digit_9") << QByteArrayLiteral("9") << 10;

	// Shifted digits
	QTest::addRow("exclam") << QByteArrayLiteral("!") << 2;
	QTest::addRow("at") << QByteArrayLiteral("@") << 3;
	QTest::addRow("numbersign") << QByteArrayLiteral("#") << 4;
	QTest::addRow("dollar") << QByteArrayLiteral("$") << 5;
	QTest::addRow("percent") << QByteArrayLiteral("%") << 6;
	QTest::addRow("asciicircum") << QByteArrayLiteral("^") << 7;
	QTest::addRow("ampersand") << QByteArrayLiteral("&") << 8;
	QTest::addRow("asterisk") << QByteArrayLiteral("*") << 9;
	QTest::addRow("parenleft") << QByteArrayLiteral("(") << 10;
	QTest::addRow("parenright") << QByteArrayLiteral(")") << 11;

	// Minus/underscore and equal/plus
	QTest::addRow("minus") << QByteArrayLiteral("-") << 12;
	QTest::addRow("underscore") << QByteArrayLiteral("_") << 12;
	QTest::addRow("equal") << QByteArrayLiteral("=") << 13;
	QTest::addRow("plus") << QByteArrayLiteral("+") << 13;

	// Brackets
	QTest::addRow("bracketleft") << QByteArrayLiteral("[") << 26;
	QTest::addRow("braceleft") << QByteArrayLiteral("{") << 26;
	QTest::addRow("bracketright") << QByteArrayLiteral("]") << 27;
	QTest::addRow("braceright") << QByteArrayLiteral("}") << 27;

	// Semicolon/colon and apostrophe/quote
	QTest::addRow("semicolon") << QByteArrayLiteral(";") << 39;
	QTest::addRow("colon") << QByteArrayLiteral(":") << 39;
	QTest::addRow("apostrophe") << QByteArrayLiteral("'") << 40;
	QTest::addRow("quotedbl") << QByteArrayLiteral("\"") << 40;

	// Grave/tilde and backslash/bar
	QTest::addRow("grave") << QByteArrayLiteral("`") << 41;
	QTest::addRow("asciitilde") << QByteArrayLiteral("~") << 41;
	QTest::addRow("backslash") << QByteArrayLiteral("\\") << 43;
	QTest::addRow("bar") << QByteArrayLiteral("|") << 43;

	// Comma/less, period/greater, slash/question
	QTest::addRow("comma") << QByteArrayLiteral(",") << 51;
	QTest::addRow("less") << QByteArrayLiteral("<") << 51;
	QTest::addRow("period") << QByteArrayLiteral(".") << 52;
	QTest::addRow("greater") << QByteArrayLiteral(">") << 52;
	QTest::addRow("slash") << QByteArrayLiteral("/") << 53;
	QTest::addRow("question") << QByteArrayLiteral("?") << 53;

	// Whitespace
	QTest::addRow("space") << QByteArrayLiteral(" ") << 57;
	QTest::addRow("newline") << QByteArrayLiteral("\n") << 28;
	QTest::addRow("tab") << QByteArrayLiteral("\t") << 15;
}

void LinuxKeyboardMappingTest::testUtf8ToLinuxKeycode()
{
	QFETCH(QByteArray, utf8);
	QFETCH(int, expectedKeycode);

	QCOMPARE(LinuxKeyboardInput::utf8ToLinuxKeycode(utf8), expectedKeycode);
}

void LinuxKeyboardMappingTest::testUnmappedKeysymReturnsMinusOne()
{
	QCOMPARE(LinuxKeyboardInput::keysymToLinuxKeycode(0xdeadbeef), -1);
}

void LinuxKeyboardMappingTest::testUnmappedUtf8ReturnsMinusOne()
{
	QCOMPARE(LinuxKeyboardInput::utf8ToLinuxKeycode(QByteArrayLiteral("\xc3\xa9")), -1);
}

QTEST_MAIN(LinuxKeyboardMappingTest)

#include "LinuxKeyboardMappingTest.moc"
