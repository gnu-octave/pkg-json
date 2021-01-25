////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2020 The Octave Project Developers
//
// See the file COPYRIGHT.md in the top-level directory of this
// distribution or <https://octave.org/copyright/>.
//
// This file is part of Octave.
//
// Octave is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Octave is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Octave; see the file COPYING.  If not, see
// <https://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////////////

#ifndef OCTAVE_7_H__
#define OCTAVE_7_H__

#include <octave/builtin-defun-decls.h>
#include <octave/lex.h>
#include <octave/oct-string.h>
#include <octave/version.h>

#if OCTAVE_MAJOR_VERSION < 7

namespace octave
{
  //! Helper class for `make_valid_name` function calls.
  //!
  //! Extracting options separately for multiple (e.g. 1000+) function calls
  //! avoids expensive repetitive parsing of the very same options.

  class
  OCTINTERP_API
  make_valid_name_options
  {
  public:

    //! Default options for `make_valid_name` function calls.
    //!
    //! Calling the constructor without arguments is equivalent to:
    //!
    //! @code{.cc}
    //! make_valid_name_options (ovl ("ReplacementStyle", "underscore",
    //!                               "Prefix", "x"));
    //! @endcode

    make_valid_name_options () = default;

    //! Extract attribute-value-pairs from an octave_value_list of strings.
    //!
    //! If attributes occur multiple times, the rightmost pair is chosen.
    //!
    //! @code{.cc}
    //! make_valid_name_options (ovl ("ReplacementStyle", "hex", ...));
    //! @endcode

    make_valid_name_options (const octave_value_list& args);

    //! @return ReplacementStyle, see `help matlab.lang.makeValidName`.

    const std::string&
    get_replacement_style () const { return m_replacement_style; }

    //! @return Prefix, see `help matlab.lang.makeValidName`.

    const std::string& get_prefix () const { return m_prefix; }

  private:

    std::string m_replacement_style{"underscore"};
    std::string m_prefix{"x"};
  };

  bool
  make_valid_name (std::string& str, const make_valid_name_options& options)
  {
    // If `isvarname (str)`, no modifications necessary.
    if (valid_identifier (str) && ! iskeyword (str))
      return false;

    // Change whitespace followed by lowercase letter to uppercase, except
    // for the first
    bool previous = false;
    bool any_non_space = false;
    for (char& c : str)
      {
        c = ((any_non_space && previous && std::isalpha (c)) ? std::toupper (c)
                                                             : c);
        previous = std::isspace (c);
        any_non_space |= (! previous);  // once true, always true
      }

    // Remove any whitespace.
    str.erase (std::remove_if (str.begin(), str.end(),
                               [] (unsigned char x)
                                  { return std::isspace(x); }),
               str.end());
    if (str.empty ())
      str = options.get_prefix ();

    // Add prefix and capitalize first character, if `str` is a reserved
    // keyword.
    if (iskeyword (str))
      {
        str[0] = std::toupper (str[0]);
        str = options.get_prefix () + str;
      }

    // Add prefix if first character is not a letter or underscore.
    if (! std::isalpha (str[0]) && str[0] != '_')
      str = options.get_prefix () + str;

    // Replace non alphanumerics or underscores
    if (options.get_replacement_style () == "underscore")
      for (char& c : str)
        c = (std::isalnum (c) ? c : '_');
    else if (options.get_replacement_style () == "delete")
      str.erase (std::remove_if (str.begin(), str.end(),
                                 [] (unsigned char x)
                                    { return ! std::isalnum (x) && x != '_'; }),
                 str.end());
    else if (options.get_replacement_style () == "hex")
      {
        const std::string permitted_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                            "abcdefghijklmnopqrstuvwxyz"
                                            "_0123456789";
        // Get the first non-permitted char.
        size_t pos = str.find_first_not_of (permitted_chars);
        // Buffer for hex string "0xFF" (+1 for null terminator).
        char hex_str[5];
        // Repeat until end of string.
        while (pos != std::string::npos)
          {
            // Replace non-permitted char by it's hex value.
            std::snprintf (hex_str, sizeof (hex_str), "0x%02X", str[pos]);
            str.replace (pos, 1, hex_str);
            // Get the next occurrence from the last position.
            // (-1 for null terminator)
            pos = str.find_first_not_of (permitted_chars,
                                         pos + sizeof (hex_str) - 1);
          }
      }

    return true;
  }

  make_valid_name_options::make_valid_name_options
    (const octave_value_list& args)
  {
    auto nargs = args.length ();
    if (nargs == 0)
      return;

    // nargs = 2, 4, 6, ... permitted
    if (nargs % 2)
      error ("makeValidName: property/value options must occur in pairs");

    auto str_to_lower = [] (std::string& s)
                           {
                             std::transform (s.begin(), s.end(), s.begin(),
                                             [] (unsigned char c)
                                                { return std::tolower(c); });
                           };

    for (auto i = 0; i < nargs; i = i + 2)
      {
        std::string parameter = args(i).xstring_value ("makeValidName: "
          "option argument must be a string");
        str_to_lower (parameter);
        if (parameter == "replacementstyle")
          {
            m_replacement_style = args(i + 1).xstring_value ("makeValidName: "
              "'ReplacementStyle' value must be a string");
            str_to_lower (m_replacement_style);
            if ((m_replacement_style != "underscore")
                && (m_replacement_style != "delete")
                && (m_replacement_style != "hex"))
              error ("makeValidName: invalid 'ReplacementStyle' value '%s'",
                     m_replacement_style.c_str ());
          }
        else if (parameter == "prefix")
          {
            m_prefix = args(i + 1).xstring_value ("makeValidName: "
              "'Prefix' value must be a string");
            if (! octave::valid_identifier (m_prefix)
                || octave::iskeyword (m_prefix))
              error ("makeValidName: invalid 'Prefix' value '%s'",
                     m_prefix.c_str ());
          }
        else
          error ("makeValidName: unknown property '%s'", parameter.c_str ());
      }
  }

#if OCTAVE_MAJOR_VERSION < 6

  // In most cases, the following are preferred for efficiency.  Some
  // cases may require the flexibility of the general unwind_protect
  // mechanism defined above.

  // Perform action at end of the current scope when unwind_action
  // object destructor is called.
  //
  // For example:
  //
  //   void fcn (int val) { ... }
  //
  // ...
  //
  //   {
  //     int val = 42;
  //
  //     // template parameters, std::bind and std::function provide
  //     // flexibility in calling forms (function pointer or lambda):
  //
  //     unwind_action act1 (fcn, val);
  //     unwind_action act2 ([val] (void) { fcn (val); });
  //   }
  //
  // NOTE: Don't forget to provide a name for the unwind_action
  // variable.  If you write
  //
  //   unwind_action /* NO NAME! */ (...);
  //
  // then the destructor for the temporary anonymous object will be
  // called immediately after the object is constructed instead of at
  // the end of the current scope.

  class OCTAVE_API unwind_action
  {
  public:

    unwind_action (void) : m_fcn () { }

    // FIXME: Do we need to apply std::forward to the arguments to
    // std::bind here?

    template <typename F, typename... Args>
    unwind_action (F&& fcn, Args&&... args)
      : m_fcn (std::bind (fcn, args...))
    { }

    // No copying!

    unwind_action (const unwind_action&) = delete;

    unwind_action& operator = (const unwind_action&) = delete;

    ~unwind_action (void) { run (); }

    // FIXME: Do we need to apply std::forward to the arguments to
    // std::bind here?

    template <typename F, typename... Args>
    void set (F&& fcn, Args&&... args)
    {
      m_fcn = std::bind (fcn, args...);
    }

    void set (void) { m_fcn = nullptr; }

    // Alias for set() which is clearer about programmer intention.
    void discard (void) { set (); }

    void run (void)
    {
      if (m_fcn)
        m_fcn ();

      // Invalidate so action won't run again when object is deleted.
      discard ();
    }

  private:

    std::function<void (void)> m_fcn;
  };

#endif

}

#endif

#endif
