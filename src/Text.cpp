#include "MarkupParser.hpp"
#include "GraphicsImpl.hpp"
#include <Gosu/Bitmap.hpp>
#include <Gosu/Graphics.hpp>
#include <Gosu/Image.hpp>
#include <Gosu/Math.hpp>
#include <Gosu/Text.hpp>
#include <Gosu/Utility.hpp>
#include <cassert>
#include <cmath>
#include <algorithm>
#include <map>
#include <vector>
using namespace std;

namespace Gosu
{
    struct WordInfo
    {
        vector<FormattedString> parts;
        bool is_whitespace;
        bool is_end_of_line;
        int width;

        WordInfo(const std::string& font_name, int font_height, vector<FormattedString> parts)
        : parts(move(parts))
        {
            assert (! parts.empty());

            width = 0;
            for (const auto& part : parts) {
                assert (! part.string.empty());
                width += text_width(part.string, font_name, font_height, part.flags);
            }

            is_whitespace  = isspace((int) parts.front().string.front());
            is_end_of_line = isspace((int) parts.back().string.back());
        }
    };

    class TextBlockBuilder
    {
        Bitmap bmp;
        int used_lines = 0;
        int allocated_lines = 0;

        vector<WordInfo> current_line;

        string font_name;
        int font_height;
        int line_spacing;
        Alignment align;

        void allocate_next_line()
        {
            ++used_lines;
            if (used_lines == allocated_lines) {
                allocated_lines += 10;
                resize_to_allocated_lines();
            }
        }

        void resize_to_allocated_lines()
        {
            bmp.resize(bmp.width(),
                       font_height * allocated_lines + line_spacing * (allocated_lines - 1));
        }

    public:
        TextBlockBuilder(string font_name, int font_height, int line_spacing,
                         int width, Alignment align)
        :   font_name(move(font_name)), font_height(font_height), line_spacing(line_spacing),
            align(align)
        {
            resize_to_allocated_lines();
        }

        int width() const
        {
            return bmp.width();
        }

        void add_current_line(bool override_align)
        {
            allocate_next_line();
            
            auto words = end - begin;
            
            int total_spacing = 0;
            if (begin < end) {
                for (auto i = begin; i != end - 1; ++i) {
                    total_spacing += i->space_width;
                }
            }

            // Where does the line start? (y)
            int top = (used_lines - 1) * (font_height + line_spacing);

            // Where does the line start? (x)
            int pos;
            switch (align) {
            // Start so that the text touches the right border.
            case AL_RIGHT:
                pos = bmp.width() - words_width - total_spacing;
                break;

            // Start so that the text is centered.
            case AL_CENTER:
                pos = bmp.width() - words_width - total_spacing;
                pos /= 2;
                break;

            // Just start at the left border.
            default:
                pos = 0;
            }
            
            for (auto cur = begin; cur != end; ++cur) {
                vector<FormattedString> parts = cur->text.split_parts();
                int x = 0;
                for (auto& part : parts) {
                    string unformatted_part = wstring_to_utf8(part.unformat());
                    draw_text(bmp, unformatted_part, pos + x, top,
                        part.color_at(0), font_name, font_height, part.flags_at(0));
                    
                    x += Gosu::text_width(unformatted_part, font_name, font_height,
                                          part.flags_at(0));
                }
                
                if (align == AL_JUSTIFY && !override_align) {
                    pos += cur->width + 1.0 * (width() - words_width) / (words - 1);
                }
                else {
                    pos += cur->width + cur->space_width;
                }
            }
        }
        
        void add_empty_line()
        {
            alloc_next_line();
        }

        const Bitmap& result()
        {
            // Shrink to fit the currently used height.
            allocated_lines = used_lines;
            resize_to_allocated_lines();
            return bmp;
        }
        
        int space_width() const
        {
            return space_width_;
        }
    };

    void process_words(TextBlockBuilder& builder, const Words& words)
    {
        if (words.empty()) return builder.add_empty_line();

        // Index into words to the first word in the current line.
        auto line_begin = words.begin();

        // Used width, in pixels, of the words [line_begin..w[.
        int words_width = 0;

        // Used width of the spaces between (w-line_begin) words.
        int spaces_width = 0;

        for (auto w = words.begin(); w != words.end(); ++w) {
            int new_words_width = words_width + w->width;

            if (new_words_width + spaces_width <= builder.width()) {
                // There's enough space for the words [line_begin..w] plus
                // the spaces between them: Proceed with the next word.
                words_width = new_words_width;
                spaces_width += w->space_width;
            }
            else {
                // No, this word wouldn't fit into the current line: Draw
                // the current line, then start a new line with the current
                // word.
                builder.add_line(line_begin, w, words_width, false);

                line_begin = w;
                words_width = w->width;
                spaces_width = w->space_width;
            }
        }

        // Draw the last line as well.
        if (words.empty() || line_begin != words.end()) {
            builder.add_line(line_begin, words.end(), words_width, true);
        }
    }
    
    void process_paragraph(TextBlockBuilder& builder, const FormattedString& paragraph)
    {
        Words collected_words;
        
        int begin_of_word = 0;
        
        for (unsigned cur = 0; cur < paragraph.length(); ++cur) {
            WordInfo new_word;

            if (paragraph.char_at(cur) == L' ') {
                // Whitespace:
                // Add last word to list if existent
                if (begin_of_word != cur) {
                    new_word.text = paragraph.range(begin_of_word, cur);
                    new_word.width = builder.text_width(new_word.text);
                    new_word.space_width = builder.space_width();
                    collected_words.push_back(new_word);
                }
                begin_of_word = cur + 1;
            }
            else if (is_breaking_asian_glyph(paragraph.char_at(cur))) {
                // Asian glyph (treat as single word):
                // Add last word to list if existent
                if (begin_of_word != cur) {
                    new_word.text = paragraph.range(begin_of_word, cur);
                    new_word.width = builder.text_width(new_word.text);
                    new_word.space_width = 0;
                    collected_words.push_back(new_word);
                }
                // Add glyph as a single "word"
                new_word.text = paragraph.range(cur, cur + 1);
                new_word.width = builder.text_width(new_word.text);
                new_word.space_width = 0;
                collected_words.push_back(new_word);
                begin_of_word = cur + 1;
            }
        }
        if (begin_of_word < paragraph.length()) {
            WordInfo last_word;
            last_word.text = paragraph.range(begin_of_word, paragraph.length());
            last_word.width = builder.text_width(last_word.text);
            last_word.space_width = 0;
            collected_words.push_back(last_word);
        }
        
        process_words(builder, collected_words);
    }

    void process_text(TextBlockBuilder& builder, const FormattedString& text)
    {
        vector<FormattedString> paragraphs = text.split_lines();
        for (auto& paragraph : paragraphs) {
            process_paragraph(builder, paragraph);
        }
    }
}
}

Gosu::Bitmap Gosu::create_text(const string& text, const string& font_name, int font_height,
                               int line_spacing, int width, Alignment align, unsigned font_flags)
{
    if (font_height <= 0)            throw invalid_argument("font_height must be > 0");
    if (width <= 0)                  throw invalid_argument("width must be > 0");
    if (line_spacing < -font_height) throw invalid_argument("line_spacing must be ≥ -font_height");

    FormattedString fs(text.c_str(), font_flags);
    
    // Set up the builder object which will manage all the drawing and
    // conversions for us.
    TextBlockBuilder builder(font_name, font_height, line_spacing, width, align);
    
    // Let the process* functions draw everything.
    process_text(builder, fs);
    
    // Done!
    return builder.result();
}

Gosu::Bitmap Gosu::create_text(const string& text, const string& font_name, int font_height,
                               unsigned font_flags)
{
    if (font_height <= 0) throw invalid_argument("font_height must be > 0");

    Bitmap result;

    MarkupParser parser(text.c_str(), font_flags, false, [&result](auto& substrings) {
        int line_width = 0;
        for (auto& substring : substrings) {
            width += text_width(substring.text, font_name, font_height, substring.flags);
        }
    });

    FormattedString fs(text.c_str(), font_flags);
    
    vector<FormattedString> lines = fs.split_lines();
    
    Bitmap bmp(1, static_cast<int>(lines.size() * font_height));
    
    for (int i = 0; i < lines.size(); ++i) {
        if (lines[i].length() == 0) continue;
        
        int x = 0;
        vector<FormattedString> parts = lines[i].split_parts();
        for (auto& part : parts) {
            if (part.length() == 1 && part.entity_at(0)) {
                Bitmap entity = entity_bitmap(part.entity_at(0));
                multiply_bitmap_alpha(entity, part.color_at(0).alpha());
                bmp.resize(max(bmp.width(), x + entity.width()), bmp.height(), 0x00ffffff);
                bmp.insert(entity, x, i * font_height);
                x += entity.width();
                continue;
            }

            assert (part.length() > 0);
            string unformatted_text = wstring_to_utf8(part.unformat());
            int part_width = text_width(unformatted_text, font_name, font_height,
                                        part.flags_at(0));
            bmp.resize(max<int>(bmp.width(), x + part_width), bmp.height(), 0x00ffffff);
            draw_text(bmp, unformatted_text, x, i * font_height, part.color_at(0), font_name,
                      font_height, part.flags_at(0));
            x += part_width;
        }
    }
    
    return bmp;
}
