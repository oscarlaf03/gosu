#include "MarkupParser.hpp"
#include "GraphicsImpl.hpp"
#include <Gosu/Font.hpp>
#include <Gosu/Graphics.hpp>
#include <Gosu/Image.hpp>
#include <Gosu/Math.hpp>
#include <Gosu/Text.hpp>
#include <Gosu/Utility.hpp>
#include <array>
#include <cassert>
#include <map>
using namespace std;

struct Gosu::Font::Impl
{
    string name;
    int height;
    unsigned flags;

    // The most common characters are stored directly in an array for maximum performance.
    // (It probably makes sense to just store the full Basic Multilingual Plane in here.)
    array<Gosu::Image, 128> ascii_graphemes;
    // Everything else is looked up through a map...
    map<string, Gosu::Image> other_graphemes;
    /*
    const Image& image_at(const FormattedString& fs, unsigned i)
    {
        wchar_t wc     = fs.char_at(i);
        unsigned flags = fs.flags_at(i);
        CharInfo& info = char_info(wc, flags);
        
        if (info.image.get()) return *info.image;
        
        string char_string = wstring_to_utf8(wstring(1, wc));
        // TODO: Would be nice to have.
        // if (is_formatting_char(wc))
        //     char_string.clear();
        int char_width = Gosu::text_width(char_string, name, height, flags);
        
        Bitmap bitmap(char_width, height, 0x00ffffff);
        draw_text(bitmap, char_string, 0, 0, Color::WHITE, name, height, flags);
        info.image.reset(new Image(bitmap));
        info.factor = 0.5;
        return *info.image;
    }
    
    double factor_at(const FormattedString& fs, unsigned index)
    {
        return char_info(fs.char_at(index), fs.flags_at(index)).factor;
    }*/
};

Gosu::Font::Font(int font_height, const string& font_name, unsigned font_flags)
: pimpl(new Impl)
{
    pimpl->name = font_name;
    pimpl->height = font_height * 2;
    pimpl->flags = font_flags;
}

string Gosu::Font::name() const
{
    return pimpl->name;
}

int Gosu::Font::height() const
{
    return pimpl->height / 2;
}

unsigned Gosu::Font::flags() const
{
    return pimpl->flags;
}
double Gosu::Font::text_width(const string& text, double scale_x) const
{
/*
    wstring wtext = utf8_to_wstring(text);
    FormattedString fs(wtext.c_str(), flags());
    double result = 0;
    for (unsigned i = 0; i < fs.length(); ++i) {
        const Image& image = pimpl->image_at(fs, i);
        double factor = pimpl->factor_at(fs, i);
        result += image.width() * factor;
    }
    return result * scale_x;*/ return 0;
}

void Gosu::Font::draw(const string& text, double x, double y, ZPos z,
    double scale_x, double scale_y, Color c, AlphaMode mode) const
{
/*    wstring wtext = utf8_to_wstring(text);
    FormattedString fs(wtext.c_str(), flags());
    
    for (unsigned i = 0; i < fs.length(); ++i) {
        const Image& image = pimpl->image_at(fs, i);
        double factor = pimpl->factor_at(fs, i);
        Color color = multiply(fs.color_at(i), c);
        image.draw(x, y, z, scale_x * factor, scale_y * factor, color, mode);
        x += image.width() * scale_x * factor;
    }*/
}

void Gosu::Font::draw_rel(const string& text, double x, double y, ZPos z,
    double rel_x, double rel_y, double scale_x, double scale_y, Color c, AlphaMode mode) const
{
    x -= text_width(text) * scale_x * rel_x;
    y -= height() * scale_y * rel_y;
    
    draw(text, x, y, z, scale_x, scale_y, c, mode);
}

void Gosu::Font::set_image(wchar_t wc, const Image& image)
{
    for (unsigned flags = 0; flags < FF_COMBINATIONS; ++flags) {
//        set_image(wc, flags, image);
    }
}

void Gosu::Font::set_image(wchar_t wc, unsigned font_flags, const Image& image)
{
/*    Impl::CharInfo& ci = pimpl->char_info(wc, font_flags);
    if (ci.image.get()) throw logic_error("Cannot set image for the same character twice");
    ci.image.reset(new Image(image));
    ci.factor = 1.0;*/
}

void Gosu::Font::draw_rot(const string& text, double x, double y, ZPos z, double angle,
    double scale_x, double scale_y, Color c, AlphaMode mode) const
{
    Graphics::transform(rotate(angle, x, y), [&] {
        draw(text, x, y, z, scale_x, scale_y, c, mode);
    });
}

