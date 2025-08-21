// 判断字符属于哪种语言类型（简易实现，可扩展）
#include "unicodetable.h"

namespace filemanager {
std::string getLanguageType(char32_t ch)
    {
        // 0x0000-0xFFFF 0 基本多文種平面
        if (ch >= 0x0000 && ch <= 0x007F)
            return "Basic Latin alphabet";
        if (ch >= 0x0080 && ch <= 0x00FF)
            return "Latin-1 Supplement";
        if (ch >= 0x0100 && ch <= 0x017F)
            return "Latin Extended-A";
        if (ch >= 0x0180 && ch <= 0x024F)
            return "Latin Extended-B";
        if (ch >= 0x0250 && ch <= 0x02AF)
            return "IPA Extensions";
        if (ch >= 0x02B0 && ch <= 0x02FF)
            return "Spacing Modifier Letters";
        if (ch >= 0x0300 && ch <= 0x036F)
            return "Combining Diacritical Marks";
        if (ch >= 0x0370 && ch <= 0x03FF)
            return "Greek and Coptic";
        if (ch >= 0x0400 && ch <= 0x04FF)
            return "Cyrillic";
        if (ch >= 0x0500 && ch <= 0x052F)
            return "Cyrillic Supplement";
        if (ch >= 0x0530 && ch <= 0x058F)
            return "Armenian";
        if (ch >= 0x0590 && ch <= 0x05FF)
            return "Hebrew";
        if (ch >= 0x0600 && ch <= 0x06FF)
            return "Arabic";
        if (ch >= 0x0700 && ch <= 0x074F)
            return "Syriac";
        if (ch >= 0x0750 && ch <= 0x077F)
            return "Arabic Supplement";
        if (ch >= 0x0780 && ch <= 0x07BF)
            return "Thaana";
        if (ch >= 0x07C0 && ch <= 0x07FF)
            return "West African written language";
        if (ch >= 0x0800 && ch <= 0x083F)
            return "Samaritan";
        if (ch >= 0x0840 && ch <= 0x085F)
            return "Mandaic";
        if (ch >= 0x0860 && ch <= 0x086F)
            return "Syriac Supplement";
        if (ch >= 0x0870 && ch <= 0x089F)
            return "Arabic Extended-B";
        if (ch >= 0x08A0 && ch <= 0x08FF)
            return "Arabic Extended-A";
        if (ch >= 0x0900 && ch <= 0x097F)
            return "Devanagari";
        if (ch >= 0x0980 && ch <= 0x09FF)
            return "Bengali";
        if (ch >= 0x0A00 && ch <= 0x0A7F)
            return "Gurmukhi";
        if (ch >= 0x0A80 && ch <= 0x0AFF)
            return "Gujarati";
        if (ch >= 0x0B00 && ch <= 0x0B7F)
            return "Oriya";
        if (ch >= 0x0B80 && ch <= 0x0BFF)
            return "Tamil";
        if (ch >= 0x0C00 && ch <= 0x0C7F)
            return "Telugu";
        if (ch >= 0x0C80 && ch <= 0x0CFF)
            return "Kannada";
        if (ch >= 0x0D00 && ch <= 0x0D7F)
            return "Malayalam";
        if (ch >= 0x0D80 && ch <= 0x0DFF)
            return "Sinhala";
        if (ch >= 0x0E00 && ch <= 0x0E7F)
            return "Thai";
        if (ch >= 0x0E80 && ch <= 0x0EFF)
            return "Lao";
        if (ch >= 0x0F00 && ch <= 0x0FFF)
            return "Tibetan";
        if (ch >= 0x1000 && ch <= 0x109F)
            return "Myanmar";
        if (ch >= 0x10A0 && ch <= 0x10FF)
            return "Georgian";
        if (ch >= 0x1100 && ch <= 0x11FF)
            return "Hangul Jamo";
        if (ch >= 0x1200 && ch <= 0x137F)
            return "Ethiopic";
        if (ch >= 0x1380 && ch <= 0x139F)
            return "Ethiopic Supplement";
        if (ch >= 0x13A0 && ch <= 0x13FF)
            return "Cherokee";
        if (ch >= 0x1400 && ch <= 0x167F)
            return "Unified Canadian Aboriginal Syllabics";
        if (ch >= 0x1680 && ch <= 0x169F)
            return "Ogham";
        if (ch >= 0x16A0 && ch <= 0x16FF)
            return "Runic";
        if (ch >= 0x1700 && ch <= 0x171F)
            return "Tagalog";
        if (ch >= 0x1720 && ch <= 0x173F)
            return "Hanunoo";
        if (ch >= 0x1740 && ch <= 0x175F)
            return "Buhid";
        if (ch >= 0x1760 && ch <= 0x177F)
            return "Tagbanwa";
        if (ch >= 0x1780 && ch <= 0x17FF)
            return "Khmer";
        if (ch >= 0x1800 && ch <= 0x18AF)
            return "Mongolian";
        if (ch >= 0x18B0 && ch <= 0x18FF)
            return "Unified Canadian Aboriginal Syllabics Extended";
        if (ch >= 0x1900 && ch <= 0x194F)
            return "Limbu";
        if (ch >= 0x1950 && ch <= 0x197F)
            return "Tai Le";
        if (ch >= 0x1980 && ch <= 0x19DF)
            return "New Tai Lue";
        if (ch >= 0x19E0 && ch <= 0x19FF)
            return "Khmer Symbols";
        if (ch >= 0x1A00 && ch <= 0x1A1F)
            return "Buginese";
        if (ch >= 0x1A20 && ch <= 0x1AAF)
            return "Tai Tham";
        if (ch >= 0x1AB0 && ch <= 0x1AFF)
            return "Combining Diacritical Marks Extended";
        if (ch >= 0x1B00 && ch <= 0x1B7F)
            return "Balinese";
        if (ch >= 0x1B80 && ch <= 0x1BBF)
            return "Sundanese";
        if (ch >= 0x1BC0 && ch <= 0x1BFF)
            return "Batak";
        if (ch >= 0x1C00 && ch <= 0x1C4F)
            return "Lepcha";
        if (ch >= 0x1C50 && ch <= 0x1C7F)
            return "Ol Chiki";
        if (ch >= 0x1C80 && ch <= 0x1C8F)
            return "Cyrillic Extended-C";
        if (ch >= 0x1C90 && ch <= 0x1CBF)
            return "Georgian Extended";
        if (ch >= 0x1CC0 && ch <= 0x1CCF)
            return "Sundanese Supplement";
        if (ch >= 0x1CD0 && ch <= 0x1CFF)
            return "Vedic Extensions";
        if (ch >= 0x1D00 && ch <= 0x1D7F)
            return "Phonetic Extensions";
        if (ch >= 0x1D80 && ch <= 0x1DBF)
            return "Phonetic Extensions Supplement";
        if (ch >= 0x1DC0 && ch <= 0x1DFF)
            return "Combining Diacritical Marks Supplement";
        if (ch >= 0x1E00 && ch <= 0x1EFF)
            return "Latin Extended Additional";
        if (ch >= 0x1F00 && ch <= 0x1FFF)
            return "Greek Extended";
        if (ch >= 0x2000 && ch <= 0x206F)
            return "General Punctuation";
        if (ch >= 0x2070 && ch <= 0x209F)
            return "Superscripts and Subscripts";
        if (ch >= 0x20A0 && ch <= 0x20CF)
            return "Currency Symbols";
        if (ch >= 0x20D0 && ch <= 0x20FF)
            return "Combining Diacritical Marks for Symbols";
        if (ch >= 0x2100 && ch <= 0x214F)
            return "Letterlike Symbols";
        if (ch >= 0x2150 && ch <= 0x218F)
            return "Number Forms";
        if (ch >= 0x2190 && ch <= 0x21FF)
            return "Arrows";
        if (ch >= 0x2200 && ch <= 0x22FF)
            return "Mathematical Operators";
        if (ch >= 0x2300 && ch <= 0x23FF)
            return "Miscellaneous Technical";
        if (ch >= 0x2400 && ch <= 0x243F)
            return "Control Pictures";
        if (ch >= 0x2440 && ch <= 0x245F)
            return "Optical Character Recognition";
        if (ch >= 0x2460 && ch <= 0x24FF)
            return "Enclosed Alphanumerics";
        if (ch >= 0x2500 && ch <= 0x257F)
            return "Table Symbols";
        if (ch >= 0x2580 && ch <= 0x259F)
            return "Block Elements";
        if (ch >= 0x25A0 && ch <= 0x25FF)
            return "Geometric Shapes";
        if (ch >= 0x2600 && ch <= 0x26FF)
            return "Miscellaneous Symbols";
        if (ch >= 0x2700 && ch <= 0x27BF)
            return "Printing Punctuation";
        if (ch >= 0x27C0 && ch <= 0x27EF)
            return "Miscellaneous Mathematical Symbols-A";
        if (ch >= 0x27F0 && ch <= 0x27FF)
            return "Supplemental Arrows-A";
        if (ch >= 0x2800 && ch <= 0x28FF)
            return "Braille Patterns";
        if (ch >= 0x2900 && ch <= 0x297F)
            return "Supplemental Arrows-B";
        if (ch >= 0x2980 && ch <= 0x29FF)
            return "Miscellaneous Mathematical Symbols-B";
        if (ch >= 0x2A00 && ch <= 0x2AFF)
            return "Supplemental Mathematical Operators";
        if (ch >= 0x2B00 && ch <= 0x2BFF)
            return "Miscellaneous Symbols and Arrows";
        if (ch >= 0x2C00 && ch <= 0x2C5F)
            return "Glagolitic";
        if (ch >= 0x2C60 && ch <= 0x2C7F)
            return "Latin Extended-C";
        if (ch >= 0x2C80 && ch <= 0x2CFF)
            return "Coptic";
        if (ch >= 0x2D00 && ch <= 0x2D2F)
            return "Georgian Supplement";
        if (ch >= 0x2D30 && ch <= 0x2D7F)
            return "Tifinagh";
        if (ch >= 0x2D80 && ch <= 0x2DDF)
            return "Ethiopic Extended";
        if (ch >= 0x2DE0 && ch <= 0x2DFF)
            return "Cyrillic Extended-A";
        if (ch >= 0x2E00 && ch <= 0x2E7F)
            return "Supplemental Punctuation";
        if (ch >= 0x2E80 && ch <= 0x2EFF)
            return "CJK Radicals Supplement";
        if (ch >= 0x2F00 && ch <= 0x2FDF)
            return "Kangxi Radicals";
        if (ch >= 0x2FE0 && ch <= 0x2FEF)
            return "Undefined";
        if (ch >= 0x2FF0 && ch <= 0x2FFF)
            return "Ideographic Description Characters";
        if (ch >= 0x3000 && ch <= 0x303F)
            return "CJK Symbols and Punctuation";
        if (ch >= 0x3040 && ch <= 0x309F)
            return "Hiragana";
        if (ch >= 0x30A0 && ch <= 0x30FF)
            return "Katakana";
        if (ch >= 0x3100 && ch <= 0x312F)
            return "Bopomofo";
        if (ch >= 0x3130 && ch <= 0x318F)
            return "Hangul Compatibility Jamo";
        if (ch >= 0x3190 && ch <= 0x319F)
            return "Annotated pictographic symbols";
        if (ch >= 0x31A0 && ch <= 0x31BF)
            return "Bopomofo Extended";
        if (ch >= 0x31C0 && ch <= 0x31EF)
            return "CJK Strokes";
        if (ch >= 0x31F0 && ch <= 0x31FF)
            return "Katakana Phonetic Extensions";
        if (ch >= 0x3200 && ch <= 0x32FF)
            return "Enclosed CJK Letters and Months";
        if (ch >= 0x3300 && ch <= 0x33FF)
            return "CJK Compatibility";
        if (ch >= 0x3400 && ch <= 0x4DBF)
            return "CJK Unified Ideographs Extension A";
        if (ch >= 0x4DC0 && ch <= 0x4DFF)
            return "Yijing Hexagram Symbols";
        if (ch >= 0x4E00 && ch <= 0x9FFF)
            return "CJK Unified Ideographs";
        if (ch >= 0xA000 && ch <= 0xA48F)
            return "Yi Syllables";
        if (ch >= 0xA490 && ch <= 0xA4CF)
            return "Yi Radicals";
        if (ch >= 0xA4D0 && ch <= 0xA4FF)
            return "Lisu";
        if (ch >= 0xA500 && ch <= 0xA63F)
            return "Vai";
        if (ch >= 0xA640 && ch <= 0xA69F)
            return "Cyrillic Extended-B";
        if (ch >= 0xA6A0 && ch <= 0xA6FF)
            return "Bamum";
        if (ch >= 0xA700 && ch <= 0xA71F)
            return "Modifier Tone Letters";
        if (ch >= 0xA720 && ch <= 0xA7FF)
            return "Latin Extended-D";
        if (ch >= 0xA800 && ch <= 0xA82F)
            return "Syloti Nagri";
        if (ch >= 0xA830 && ch <= 0xA83F)
            return "Common Indic Number Forms";
        if (ch >= 0xA840 && ch <= 0xA87F)
            return "Phags-pa";
        if (ch >= 0xA880 && ch <= 0xA8DF)
            return "Saurashtra";
        if (ch >= 0xA8E0 && ch <= 0xA8FF)
            return "Devanagari";
        if (ch >= 0xA900 && ch <= 0xA92F)
            return "Kayah Li";
        if (ch >= 0xA930 && ch <= 0xA95F)
            return "Rejang";
        if (ch >= 0xA960 && ch <= 0xA97F)
            return "Hangul Jamo Extended-A";
        if (ch >= 0xA980 && ch <= 0xA9DF)
            return "Javanese";
        if (ch >= 0xA9E0 && ch <= 0xA9FF)
            return "Myanmar Extended-B";
        if (ch >= 0xAA00 && ch <= 0xAA5F)
            return "Cham";
        if (ch >= 0xAA60 && ch <= 0xAA7F)
            return "Myanmar Extended-A";
        if (ch >= 0xAA80 && ch <= 0xAADF)
            return "Tai Viet";
        if (ch >= 0xAAE0 && ch <= 0xAAFF)
            return "Meetei Mayek Extensions";
        if (ch >= 0xAB00 && ch <= 0xAB2F)
            return "Ethiopic Extended-A";
        if (ch >= 0xAB30 && ch <= 0xAB6F)
            return "Latin Extended-E";
        if (ch >= 0xAB70 && ch <= 0xABBF)
            return "Cherokee Supplement";
        if (ch >= 0xABC0 && ch <= 0xABFF)
            return "Meetei Mayek";
        if (ch >= 0xAC00 && ch <= 0xD7AF)
            return "Hangul Syllables";
        if (ch >= 0xD7B0 && ch <= 0xD7FF)
            return "Hangul Jamo Extended-B";
        if (ch >= 0xD800 && ch <= 0xDB7F)
            return "High Surrogates";
        if (ch >= 0xDB80 && ch <= 0xDBFF)
            return "High Private Use Surrogates";
        if (ch >= 0xDC00 && ch <= 0xDFFF)
            return "Low Surrogates";
        if (ch >= 0xE000 && ch <= 0xF8FF)
            return "Private Use Area";
        if (ch >= 0xF900 && ch <= 0xFAFF)
            return "CJK Compatibility Ideographs";
        if (ch >= 0xFB00 && ch <= 0xFB4F)
            return "Alphabetic Presentation Forms";
        if (ch >= 0xFB50 && ch <= 0xFDFF)
            return "Arabic Presentation Forms-A";
        if (ch >= 0xFE00 && ch <= 0xFE0F)
            return "Variation Selectors";
        if (ch >= 0xFE10 && ch <= 0xFE1F)
            return "Vertical Forms";
        if (ch >= 0xFE20 && ch <= 0xFE2F)
            return "Combining Half Marks";
        if (ch >= 0xFE30 && ch <= 0xFE4F)
            return "CJK Compatibility Forms";
        if (ch >= 0xFE50 && ch <= 0xFE6F)
            return "Small Form Variants";
        if (ch >= 0xFE70 && ch <= 0xFEFF)
            return "Arabic Presentation Forms-B";
        if (ch >= 0xFF00 && ch <= 0xFFEF)
            return "Halfwidth and Fullwidth Forms";
        if (ch >= 0xFFF0 && ch <= 0xFFFF)
            return "Specials";
        // 0x10000 - 0x1FFFF 1 多文種補充平面
        if (ch >= 0x10000 && ch <= 0x1007F)
            return "Linear B";
        if (ch >= 0x10080 && ch <= 0x100FF)
            return "Linear B Syllabary";
        if (ch >= 0x10100 && ch <= 0x1013F)
            return "Aegean Numbers";
        if (ch >= 0x10140 && ch <= 0x1018F)
            return "Ancient Greek Numbers";
        if (ch >= 0x10190 && ch <= 0x101CF)
            return "Ancient Symbols";
        if (ch >= 0x101D0 && ch <= 0x101FF)
            return "Phaistos Disc";
        if (ch >= 0x10200 && ch <= 0x1027F)
            return "Undefined";
        if (ch >= 0x10280 && ch <= 0x1029F)
            return "Lycian";
        if (ch >= 0x102A0 && ch <= 0x102DF)
            return "Carian";
        if (ch >= 0x102E0 && ch <= 0x102FF)
            return "Coptic Epact Numbers";
        if (ch >= 0x10300 && ch <= 0x1032F)
            return "Old Italic";
        if (ch >= 0x10330 && ch <= 0x1034F)
            return "Gothic";
        if (ch >= 0x10350 && ch <= 0x1037F)
            return "Old Permic";
        if (ch >= 0x10380 && ch <= 0x1039F)
            return "Ugaritic";
        if (ch >= 0x103A0 && ch <= 0x103DF)
            return "Old Persian";
        if (ch >= 0x103E0 && ch <= 0x103FF)
            return "Undefined";
        if (ch >= 0x10400 && ch <= 0x1044F)
            return "Deseret";
        if (ch >= 0x10450 && ch <= 0x1047F)
            return "Shavian";
        if (ch >= 0x10480 && ch <= 0x104AF)
            return "Osmanya";
        if (ch >= 0x104B0 && ch <= 0x104FF)
            return "Osage";
        if (ch >= 0x10500 && ch <= 0x1052F)
            return "Elbasan";
        if (ch >= 0x10530 && ch <= 0x1056F)
            return "Caucasian Albanian";
        if (ch >= 0x10570 && ch <= 0x105BF)
            return "Viskuchian";
        if (ch >= 0x105C0 && ch <= 0x105FF)
            return "Todhri";
        if (ch >= 0x10600 && ch <= 0x1077F)
            return "Linear A";
        if (ch >= 0x10780 && ch <= 0x107BF)
            return "Latin alphabet extension F";
        if (ch >= 0x107C0 && ch <= 0x107FF)
            return "Undefined";
        if (ch >= 0x10800 && ch <= 0x1083F)
            return "Cypriot Syllabary";
        if (ch >= 0x10840 && ch <= 0x1085F)
            return "Imperial Aramaic";
        if (ch >= 0x10860 && ch <= 0x1087F)
            return "Palmyrene";
        if (ch >= 0x10880 && ch <= 0x108AF)
            return "Nabataean";
        if (ch >= 0x108B0 && ch <= 0x108FF)
            return "Undefined";
        if (ch >= 0x108E0 && ch <= 0x108FF)
            return "Hatran";
        if (ch >= 0x10900 && ch <= 0x1091F)
            return "Phoenician";
        if (ch >= 0x10920 && ch <= 0x1093F)
            return "Lydian";
        if (ch >= 0x10940 && ch <= 0x1097F)
            return "Undefined";
        if (ch >= 0x10980 && ch <= 0x1099F)
            return "Meroitic Hieroglyphs";
        if (ch >= 0x109A0 && ch <= 0x109FF)
            return "Meroitic Cursive";
        if (ch >= 0x10A00 && ch <= 0x10A5F)
            return "Kharoshthi";
        if (ch >= 0x10A60 && ch <= 0x10A7F)
            return "Old South Arabian";
        if (ch >= 0x10A80 && ch <= 0x10A9F)
            return "Old North Arabian";
        if (ch >= 0x10AA0 && ch <= 0x10ABF)
            return "Undefined";
        if (ch >= 0x10AC0 && ch <= 0x10AFF)
            return "Manichaean";
        if (ch >= 0x10B00 && ch <= 0x10B3F)
            return "Avestan";
        if (ch >= 0x10B40 && ch <= 0x10B5F)
            return "Inscriptional Parthian";
        if (ch >= 0x10B60 && ch <= 0x10B7F)
            return "Inscriptional Pahlavi";
        if (ch >= 0x10B80 && ch <= 0x10BAF)
            return "Psalter Pahlavi";
        if (ch >= 0x10BB0 && ch <= 0x10BFF)
            return "Undefined";
        if (ch >= 0x10C00 && ch <= 0x10C4F)
            return "Old Turkic";
        if (ch >= 0x10C50 && ch <= 0x10C7F)
            return "Undefined";
        if (ch >= 0x10C80 && ch <= 0x10CFF)
            return "Old Hungarian";
        if (ch >= 0x10D00 && ch <= 0x10D3F)
            return "Hanifi Rohingya";
        if (ch >= 0x10D40 && ch <= 0x10D8F)
            return "Garay";
        if (ch >= 0x10D90 && ch <= 0x10E5F)
            return "Undefined";
        if (ch >= 0x10E60 && ch <= 0x10E7F)
            return "Rumi Numeral Symbols";
        if (ch >= 0x10E80 && ch <= 0x10EBF)
            return "Yezidi";
        if (ch >= 0x10EC0 && ch <= 0x10EFF)
            return "Arabic alphabet extension C";
        if (ch >= 0x10F00 && ch <= 0x10F2F)
            return "Old Sogdian";
        if (ch >= 0x10F30 && ch <= 0x10F6F)
            return "Sogdian";
        if (ch >= 0x10F70 && ch <= 0x10FAF)
            return "Uyghur alphabet";
        if (ch >= 0x10FB0 && ch <= 0x10FDF)
            return "Khwarezmia alphabet";
        if (ch >= 0x10FE0 && ch <= 0x10FFF)
            return "Ellie Maiwen";
        if (ch >= 0x11000 && ch <= 0x1107F)
            return "Brahmi";
        if (ch >= 0x11080 && ch <= 0x110CF)
            return "Kaithi";
        if (ch >= 0x110D0 && ch <= 0x110FF)
            return "Sora Sompeng";
        if (ch >= 0x11100 && ch <= 0x1114F)
            return "Chakma";
        if (ch >= 0x11150 && ch <= 0x1117F)
            return "Mahajani";
        if (ch >= 0x11180 && ch <= 0x111DF)
            return "Sharada";
        if (ch >= 0x111E0 && ch <= 0x111FF)
            return "Sinhala Archaic Numbers";
        if (ch >= 0x11200 && ch <= 0x1124F)
            return "Khojki";
        if (ch >= 0x11250 && ch <= 0x1127F)
            return "Undefined";
        if (ch >= 0x11280 && ch <= 0x112AF)
            return "Multani";
        if (ch >= 0x112B0 && ch <= 0x112FF)
            return "Khudawadi";
        if (ch >= 0x11300 && ch <= 0x1137F)
            return "Grantha";
        if (ch >= 0x11380 && ch <= 0x113FF)
            return "Tulu-Tigalari";
        if (ch >= 0x11400 && ch <= 0x1147F)
            return "Newa";
        if (ch >= 0x11480 && ch <= 0x114DF)
            return "Tirhuta";
        if (ch >= 0x114E0 && ch <= 0x1157F)
            return "Undefined";
        if (ch >= 0x11580 && ch <= 0x115FF)
            return "Siddham";
        if (ch >= 0x11600 && ch <= 0x1165F)
            return "Modi";
        if (ch >= 0x11660 && ch <= 0x1167F)
            return "Mongolian Supplement";
        if (ch >= 0x11680 && ch <= 0x116CF)
            return "Takri";
        if (ch >= 0x116D0 && ch <= 0x116FF)
            return "Myanmar Extended-C";
        if (ch >= 0x11700 && ch <= 0x1174F)
            return "Ahom";
        if (ch >= 0x11750 && ch <= 0x117FF)
            return "Undefined";
        if (ch >= 0x11800 && ch <= 0x1184F)
            return "Douglas";
        if (ch >= 0x11850 && ch <= 0x1189F)
            return "Undefined";
        if (ch >= 0x118A0 && ch <= 0x118FF)
            return "Warang Citi";
        if (ch >= 0x11900 && ch <= 0x1195F)
            return "Island letters";
        if (ch >= 0x11960 && ch <= 0x1199F)
            return "Undefined";
        if (ch >= 0x119A0 && ch <= 0x119FF)
            return "Nandinagari";
        if (ch >= 0x11A00 && ch <= 0x11A4F)
            return "Zanabazar Square";
        if (ch >= 0x11A50 && ch <= 0x11AAF)
            return "Pahawh Hmong";
        if (ch >= 0x11AB0 && ch <= 0x11ABF)
            return "Canadian Indigenous Syllable Script Extension A";
        if (ch >= 0x11AC0 && ch <= 0x11AFF)
            return "Pau Cin Hau";
        if (ch >= 0x11B00 && ch <= 0x11B5F)
            return "Devanagari Extended-A";
        if (ch >= 0x11B60 && ch <= 0x11BBF)
            return "Undefined";
        if (ch >= 0x11BC0 && ch <= 0x11BFF)
            return "Sunuwar";
        if (ch >= 0x11C00 && ch <= 0x11C6F)
            return "Bhaiksuki";
        if (ch >= 0x11C70 && ch <= 0x11CBF)
            return "Marchen";
        if (ch >= 0x11CC0 && ch <= 0x11CFF)
            return "Undefined";
        if (ch >= 0x11D00 && ch <= 0x11D5F)
            return "Masaram Gondi";
        if (ch >= 0x11D60 && ch <= 0x11DAF)
            return "Gunjala Gondi";
        if (ch >= 0x11DB0 && ch <= 0x11EDF)
            return "Undefined";
        if (ch >= 0x11EE0 && ch <= 0x11EFF)
            return "Wangjia Xiwen";
        if (ch >= 0x11F00 && ch <= 0x11F5F)
            return "Kavi Wen";
        if (ch >= 0x11F60 && ch <= 0x11FAF)
            return "Undefined";
        if (ch >= 0x11FB0 && ch <= 0x11FBF)
            return "Lisu";
        if (ch >= 0x11FC0 && ch <= 0x11FFF)
            return "Tamil language supplement";
        if (ch >= 0x12000 && ch <= 0x123FF)
            return "Cuneiform";
        if (ch >= 0x12400 && ch <= 0x1247F)
            return "Cuneiform Numbers and Punctuation";
        if (ch >= 0x12480 && ch <= 0x1254F)
            return "Early Dynastic Cuneiform";
        if (ch >= 0x12550 && ch <= 0x12F8F)
            return "Undefined";
        if (ch >= 0x12F90 && ch <= 0x12FFF)
            return "Cyprus Minoan script";
        if (ch >= 0x13000 && ch <= 0x1342F)
            return "Egyptian holy script";
        if (ch >= 0x13430 && ch <= 0x1345F)
            return "Format control of Egyptian holy books";
        if (ch >= 0x13460 && ch <= 0x143FF)
            return "Egyptian Hieroglyphs Extended-A";
        if (ch >= 0x14400 && ch <= 0x1467F)
            return "Anatolian Hieroglyphs";
        if (ch >= 0x14680 && ch <= 0x160FF)
            return "Undefined";
        if (ch >= 0x16100 && ch <= 0x1613F)
            return "Gurung Khema";
        if (ch >= 0x16140 && ch <= 0x167FF)
            return "Undefined";
        if (ch >= 0x16800 && ch <= 0x16A3F)
            return "Bamum";
        if (ch >= 0x16A40 && ch <= 0x16A6F)
            return "Mro";
        if (ch >= 0x16AD0 && ch <= 0x16AFF)
            return "Bassa Vah";
        if (ch >= 0x16B00 && ch <= 0x16B8F)
            return "Salvation Miao Wen";
        if (ch >= 0x16B90 && ch <= 0x16D3F)
            return "Undefined";
        if (ch >= 0x16D40 && ch <= 0x16D7F)
            return "Kirat Rai";
        if (ch >= 0x16D80 && ch <= 0x16E3F)
            return "Undefined";
        if (ch >= 0x16E40 && ch <= 0x16E9F)
            return "Medfa Idelin Wen";
        if (ch >= 0x16EA0 && ch <= 0x16EFF)
            return "Undefined";
        if (ch >= 0x16F00 && ch <= 0x16F9F)
            return "Boli Miao Wen";
        if (ch >= 0x16FA0 && ch <= 0x16FDF)
            return "Undefined";
        if (ch >= 0x16FE0 && ch <= 0x16FFF)
            return "Ideographic symbols and punctuation marks";
        if (ch >= 0x17000 && ch <= 0x187FF)
            return "Tangut";
        if (ch >= 0x18800 && ch <= 0x18AFF)
            return "Tangut Components";
        if (ch >= 0x18B00 && ch <= 0x18CFF)
            return "Khitai";
        if (ch >= 0x18D00 && ch <= 0x18D7F)
            return "Tangut Extension";
        if (ch >= 0x18D80 && ch <= 0x1AFEF)
            return "Undefined";
        if (ch >= 0x1AFF0 && ch <= 0x1AFFF)
            return "Kana Extended-B";
        if (ch >= 0x1B000 && ch <= 0x1B0FF)
            return "Kana supplement";
        if (ch >= 0x1B100 && ch <= 0x1B12F)
            return "kana Extended-A";
        if (ch >= 0x1B130 && ch <= 0x1B16F)
            return "Small Kana supplement";
        if (ch >= 0x1B170 && ch <= 0x1B2FF)
            return "Women's Script";
        if (ch >= 0x1B300 && ch <= 0x1BBFF)
            return "Undefined";
        if (ch >= 0x1BC00 && ch <= 0x1BC9F)
            return "Dupre shorthand";
        if (ch >= 0x1BCA0 && ch <= 0x1BCAF)
            return "Shorthand format control";
        if (ch >= 0x1BCB0 && ch <= 0x1CBFF)
            return "Undefined";
        if (ch >= 0x1CC00 && ch <= 0x1CEBF)
            return "Symbols for Legacy Computing Supplement";
        if (ch >= 0x1CEC0 && ch <= 0x1CEFF)
            return "Undefined";
        if (ch >= 0x1CF00 && ch <= 0x1CFCF)
            return "Zanmei Ni Song Music Symbols";
        if (ch >= 0x1CFD0 && ch <= 0x1CFFF)
            return "Undefined";
        if (ch >= 0x1D000 && ch <= 0x1D0FF)
            return "Geometric Shapes Extended";
        if (ch >= 0x1D100 && ch <= 0x1D1FF)
            return "Musical Symbols";
        if (ch >= 0x1D200 && ch <= 0x1D24F)
            return "Old Greek Musical Notation";
        if (ch >= 0x1D250 && ch <= 0x1D2BF)
            return "Undefined";
        if (ch >= 0x1D2C0 && ch <= 0x1D2DF)
            return "Kaktovik Numbers";
        if (ch >= 0x1D2E0 && ch <= 0x1D2FF)
            return "Maya numerals";
        if (ch >= 0x1D300 && ch <= 0x1D35F)
            return "Tai Xuan Jing symbols";
        if (ch >= 0x1D360 && ch <= 0x1D37F)
            return "Counting Rod Numerals";
        if (ch >= 0x1D380 && ch <= 0x1D3FF)
            return "Undefined";
        if (ch >= 0x1D400 && ch <= 0x1D7FF)
            return "Arabic Mathematical Alphabetic Symbols";
        if (ch >= 0x1D800 && ch <= 0x1DAAF)
            return "Sutton SignWriting";
        if (ch >= 0x1DAB0 && ch <= 0x1DEFF)
            return "Undefined";
        if (ch >= 0x1DF00 && ch <= 0x1DFFF)
            return "Latin Alphabetic Extended-G";
        if (ch >= 0x1E000 && ch <= 0x1E02F)
            return "Glagolitic Supplement";
        if (ch >= 0x1E030 && ch <= 0x1E08F)
            return "Cyrillic Extended-D";
        if (ch >= 0x1E090 && ch <= 0x1E0FF)
            return "Undefined";
        if (ch >= 0x1E100 && ch <= 0x1E14F)
            return "Genesis Miaowen";
        if (ch >= 0x1E150 && ch <= 0x1E28F)
            return "Undefined";
        if (ch >= 0x1E290 && ch <= 0x1E2BF)
            return "Investment article";
        if (ch >= 0x1E2C0 && ch <= 0x1E2FF)
            return "Wen Qiao alphabet";
        if (ch >= 0x1E300 && ch <= 0x1E4CF)
            return "Undefined";
        if (ch >= 0x1E4D0 && ch <= 0x1E4FF)
            return "Mundali alphabet";
        if (ch >= 0x1E500 && ch <= 0x1E5CF)
            return "Undefined";
        if (ch >= 0x1E5D0 && ch <= 0x1E5FF)
            return "Ol Onal";
        if (ch >= 0x1E600 && ch <= 0x1E7DF)
            return "Undefined";
        if (ch >= 0x1E7E0 && ch <= 0x1E7FF)
            return "Ethiopian Alphabet Extension B";
        if (ch >= 0x1E800 && ch <= 0x1E8DF)
            return "Makasar";
        if (ch >= 0x1E8E0 && ch <= 0x1E8FF)
            return "Undefined";
        if (ch >= 0x1E900 && ch <= 0x1E95F)
            return "Rumi Numeral Symbols";
        if (ch >= 0x1E960 && ch <= 0x1EC6F)
            return "Undefined";
        if (ch >= 0x1EC70 && ch <= 0x1ECBF)
            return "Indian Siamese numerals";
        if (ch >= 0x1ECC0 && ch <= 0x1ECFF)
            return "Undefined";
        if (ch >= 0x1ED00 && ch <= 0x1ED4F)
            return "Osmansiag numbers";
        if (ch >= 0x1ED50 && ch <= 0x1EDFF)
            return "Undefined";
        if (ch >= 0x1EE00 && ch <= 0x1EEFF)
            return "Arabic Mathematical Alphabetic Symbols";
        if (ch >= 0x1EF00 && ch <= 0x1EFFF)
            return "Undefined";
        if (ch >= 0x1F000 && ch <= 0x1F02F)
            return "Mahjong Tiles";
        if (ch >= 0x1F030 && ch <= 0x1F09F)
            return "Domino Tiles";
        if (ch >= 0x1F0A0 && ch <= 0x1F0FF)
            return "Playing Cards";
        if (ch >= 0x1F100 && ch <= 0x1F1FF)
            return "Enclosed Alphanumeric Supplement";
        if (ch >= 0x1F200 && ch <= 0x1F2FF)
            return "Enclosed Ideographic Supplement";
        if (ch >= 0x1F300 && ch <= 0x1F5FF)
            return "Miscellaneous Symbols and Pictographs";
        if (ch >= 0x1F600 && ch <= 0x1F64F)
            return "Emoticons";
        if (ch >= 0x1F650 && ch <= 0x1F67F)
            return "Ornamental Dingbats";
        if (ch >= 0x1F680 && ch <= 0x1F6FF)
            return "Transport and Map Symbols";
        if (ch >= 0x1F700 && ch <= 0x1F77F)
            return "Alchemical Symbols";
        if (ch >= 0x1F780 && ch <= 0x1F7FF)
            return "Geometric Shapes Extended";
        if (ch >= 0x1F800 && ch <= 0x1F8FF)
            return "Supplemental Arrows-C";
        if (ch >= 0x1F900 && ch <= 0x1F9FF)
            return "Supplemental Symbols and Pictographs";
        if (ch >= 0x1FA00 && ch <= 0x1FA6F)
            return "Chess Symbols";
        if (ch >= 0x1FA70 && ch <= 0x1FAFF)
            return "Symbols and Pictographs Extended-A";
        if (ch >= 0x1FB00 && ch <= 0x1FBFF)
            return "Symbols for Legacy Computing";
        if (ch >= 0x1FC00 && ch <= 0x1FFFF)
            return "Undefined";
        // 0x20000-0x2FFFF 2 表意文字補充平面
        if (ch >= 0x20000 && ch <= 0x2A6DF)
            return "CJK Unified Ideographs Extension B";
        if (ch >= 0x2A6E0 && ch <= 0x2A6FF)
            return "Undefined";
        if (ch >= 0x2A700 && ch <= 0x2B73F)
            return "CJK Unified Ideographs Extension C";
        if (ch >= 0x2B740 && ch <= 0x2B81F)
            return "CJK Unified Ideographs Extension D";
        if (ch >= 0x2B820 && ch <= 0x2CEAF)
            return "CJK Unified Ideographs Extension E";
        if (ch >= 0x2CEB0 && ch <= 0x2EBEF)
            return "CJK Unified Ideographs Extension F";
        if (ch >= 0x2EBF0 && ch <= 0x2EE5F)
            return "CJK Unified Ideographs Extension I";
        if (ch >= 0x2EE60 && ch <= 0x2F7FF)
            return "Undefined";
        if (ch >= 0x2F800 && ch <= 0x2FA1F)
            return "CJK Compatibility Ideographs Supplement";
        if (ch >= 0x2FA20 && ch <= 0x2FFFF)
            return "Undefined";
        // 0x30000-0x3FFFF 3 表意文字第三平面
        if (ch >= 0x30000 && ch <= 0x3134F)
            return "CJK Unified Ideographs Extension G";
        if (ch >= 0x31350 && ch <= 0x323AF)
            return "CJK Unified Ideographs Extension H";
        if (ch >= 0x323B0 && ch <= 0xDFFFF)
            return "Undefined";
        // 0x40000-0xDFFFF 4- 13 未使用
        // 0xE0000-0xEFFFF 14 特別用途補充平面
        if (ch >= 0xE0000 && ch <= 0xE007F)
            return "label";
        if (ch >= 0xE0080 && ch <= 0xE00FF)
            return "Undefined";
        if (ch >= 0xE0100 && ch <= 0xE01EF)
            return "Variant selector supplement";
        if (ch >= 0xE01F0 && ch <= 0xEFFFF)
            return "Undefined";
        // 0xF0000-0xFFFFF 15 保留作為私人使用區（A區）
        if (ch >= 0xF0000 && ch <= 0xFFFFF)
            return "Supplementary Private Use Area-A";
        // 0x100000-0x10FFFF 16 保留作為私人使用區（B區）
        if (ch >= 0x100000 && ch <= 0x10FFFF)
            return "Supplementary Private Use Area-B";
        return "Undefined";
    }

    // 将UTF-8字符串拆分为Unicode字符（返回char32_t列表）
    std::vector<char32_t> splitToUnicode(const std::string &text)
    {
        std::vector<char32_t> result;
        size_t i = 0;
        while (i < text.size())
        {
            unsigned char c = text[i];
            char32_t code = 0;
            if (c < 0x80)
            {
                code = c;
                i += 1;
            }
            else if ((c & 0xE0) == 0xC0)
            {
                code = ((c & 0x1F) << 6) | (text[i + 1] & 0x3F);
                i += 2;
            }
            else if ((c & 0xF0) == 0xE0)
            {
                code = ((c & 0x0F) << 12) | ((text[i + 1] & 0x3F) << 6) | (text[i + 2] & 0x3F);
                i += 3;
            }
            else if ((c & 0xF8) == 0xF0)
            {
                code = ((c & 0x07) << 18) | ((text[i + 1] & 0x3F) << 12) | ((text[i + 2] & 0x3F) << 6) | (text[i + 3] & 0x3F);
                i += 4;
            }
            else
            {
                // 非法字符，跳过
                i += 1;
                continue;
            }
            result.push_back(code);
        }
        return result;
    }
}