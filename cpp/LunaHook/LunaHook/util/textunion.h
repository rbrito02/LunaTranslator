#pragma once

inline size_t str_len(const char *s) { return strlen(s); }
inline size_t str_len(const wchar_t *s) { return wcslen(s); }

template <class CharT>
struct TextUnion
{
  enum
  {
    ShortTextCapacity = 0x10 / sizeof(CharT)
  };

  union
  {
    const CharT *text; // 0x0
    CharT chars[ShortTextCapacity];
  };
  size_t size, // 0x10
      capacity;

  bool isValid() const
  {
    if (size <= 0 || size > capacity)
      return false;
    const CharT *t = getText();
    return Engine::isAddressWritable(t, size) && str_len(t) == size;
  }

  const CharT *getText() const
  {
    return capacity < ShortTextCapacity ? chars : text;
  }

  void setText(const CharT *_text, size_t _size)
  {
    if (_size < ShortTextCapacity)
      ::memcpy(chars, _text, (_size + 1) * sizeof(CharT));
    else
      text = _text;
    capacity = size = _size;
  }

  void setLongText(const CharT *_text, size_t _size)
  {
    text = _text;
    size = _size;
    capacity = max(ShortTextCapacity, _size);
  }

  void setText(const std::basic_string<CharT> &text)
  {
    setText((const CharT *)text.c_str(), text.size());
  }
  void setLongText(const std::basic_string<CharT> &text)
  {
    setLongText((const CharT *)text.c_str(), text.size());
  }
};

using TextUnionA = TextUnion<char>;
using TextUnionW = TextUnion<wchar_t>;
// EOF
