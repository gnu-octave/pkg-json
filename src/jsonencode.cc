////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2020-2021 The Octave Project Developers
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

#include <octave/oct.h>

// Include some features from Octave 7.
#include "octave7.h"

#define HAVE_RAPIDJSON 1
#define HAVE_RAPIDJSON_PRETTYWRITER 1

#if defined (HAVE_RAPIDJSON)
#  include "rapidjson/stringbuffer.h"
#  include "rapidjson/writer.h"
#  if defined (HAVE_RAPIDJSON_PRETTYWRITER)
#    include <rapidjson/prettywriter.h>
#  endif
#endif

#if defined (HAVE_RAPIDJSON)

//! Encodes a scalar Octave value into a numerical JSON value.
//!
//! @param writer RapidJSON's writer that is responsible for generating JSON.
//! @param obj scalar Octave value.
//! @param ConvertInfAndNaN @c bool that converts @c Inf and @c NaN to @c null.
//!
//! @b Example:
//!
//! @code{.cc}
//! octave_value obj (7);
//! encode_numeric (writer, obj, true);
//! @endcode

template <typename T> void
encode_numeric (T& writer, const octave_value& obj,
                const bool& ConvertInfAndNaN)
{
  double value = obj.scalar_value ();

  if (obj.is_bool_scalar ())
    writer.Bool (obj.bool_value ());
  // Any numeric input from the interpreter will be in double type so in order
  // to detect ints, we will check if the floor of the input and the input are
  // equal using fabs (A - B) < epsilon method as it is more accurate.
  // If value > 999999, MATLAB will encode it in scientific notation (double)
  else if (fabs (floor (value) - value) < std::numeric_limits<double>::epsilon ()
           && value <= 999999 && value >= -999999)
    writer.Int64 (value);
  // Possibly write NULL for non-finite values (-Inf, Inf, NaN, NA)
  else if (ConvertInfAndNaN && ! octave::math::isfinite (value))
    writer.Null ();
  else if (obj.is_double_type ())
    writer.Double (value);
  else
    error ("jsonencode: unsupported type");
}

//! Encodes character vectors and arrays into JSON strings.
//!
//! @param writer RapidJSON's writer that is responsible for generating JSON.
//! @param obj character vectors or character arrays.
//! @param original_dims The original dimensions of the array being encoded.
//! @param level The level of recursion for the function.
//!
//! @b Example:
//!
//! @code{.cc}
//! octave_value obj ("foo");
//! encode_string (writer, obj, true);
//! @endcode

template <typename T> void
encode_string (T& writer, const octave_value& obj,
               const dim_vector& original_dims, int level = 0)
{
  charNDArray array = obj.char_array_value ();

  if (array.isempty ())
    writer.String ("");
  else if (array.isvector ())
    {
      // Handle the special case where the input is a vector with more than
      // 2 dimensions (e.g. cat (8, ['a'], ['c'])).  In this case, we don't
      // split the inner vectors of the input; we merge them into one.
      if (level == 0)
        {
          std::string char_vector = "";
          for (octave_idx_type i = 0; i < array.numel (); ++i)
            char_vector += array(i);
          writer.String (char_vector.c_str ());
        }
      else
        for (octave_idx_type i = 0; i < array.numel () / original_dims(1); ++i)
          {
            std::string char_vector = "";
            for (octave_idx_type k = 0; k < original_dims(1); ++k)
              char_vector += array(i * original_dims(1) + k);
            writer.String (char_vector.c_str ());
          }
    }
  else
    {
      octave_idx_type idx;
      octave_idx_type ndims = array.ndims ();
      dim_vector dims = array.dims ();

      // In this case, we already have a vector. So, we transform it to 2-D
      // vector in order to be detected by "isvector" in the recursive call
      if (dims.num_ones () == ndims - 1)
      {
        // Handle the special case when the input is a vector with more than
        // 2 dimensions (e.g. cat (8, ['a'], ['c'])).  In this case, we don't
        // add dimension brackets and treat it as if it is a vector
        if (level != 0)
          // Place an opening and a closing bracket (represents a dimension)
          // for every dimension that equals 1 until we reach the 2-D vector
          for (int i = level; i < ndims - 1; ++i)
            writer.StartArray ();

        encode_string (writer, array.as_row (), original_dims, level);

        if (level != 0)
          for (int i = level; i < ndims - 1; ++i)
            writer.EndArray ();
      }
      else
        {
          // We place an opening and a closing bracket for each dimension
          // that equals 1 to preserve the number of dimensions when decoding
          // the array after encoding it.
          if (original_dims(level) == 1 && level != 1)
          {
            writer.StartArray ();
            encode_string (writer, array, original_dims, level + 1);
            writer.EndArray ();
          }
          else
            {
              // The second dimension contains the number of the chars in
              // the char vector. We want to treat them as a one object,
              // so we replace it with 1
              dims(1) = 1;

              for (idx = 0; idx < ndims; ++idx)
                if (dims(idx) != 1)
                  break;
              // Create the dimensions that will be used to call "num2cell"
              // We called "num2cell" to divide the array to smaller sub-arrays
              // in order to encode it recursively.
              // The recursive encoding is necessary to support encoding of
              // higher-dimensional arrays.
              RowVector conversion_dims;
              conversion_dims.resize (ndims - 1);
              for (octave_idx_type i = 0; i < idx; ++i)
                conversion_dims(i) = i + 1;
              for (octave_idx_type i = idx ; i < ndims - 1; ++i)
                conversion_dims(i) = i + 2;

              octave_value_list args (obj);
              args.append (conversion_dims);

              Cell sub_arrays = Fnum2cell (args)(0).cell_value ();

              writer.StartArray ();

              for (octave_idx_type i = 0; i < sub_arrays.numel (); ++i)
                encode_string (writer, sub_arrays(i), original_dims,
                               level + 1);

              writer.EndArray ();
            }
        }
    }
}

//! Encodes a struct Octave value into a JSON object or a JSON array depending
//! on the type of the struct (scalar struct or struct array.)
//!
//! @param writer RapidJSON's writer that is responsible for generating JSON.
//! @param obj struct Octave value.
//! @param ConvertInfAndNaN @c bool that converts @c Inf and @c NaN to @c null.
//!
//! @b Example:
//!
//! @code{.cc}
//! octave_value obj (octave_map ());
//! encode_struct (writer, obj,true);
//! @endcode

template <typename T> void
encode_struct (T& writer, const octave_value& obj, const bool& ConvertInfAndNaN)
{
  octave_map struct_array = obj.map_value ();
  octave_idx_type numel = struct_array.numel ();
  bool is_array = (numel > 1);
  string_vector keys = struct_array.keys ();

  if (is_array)
    writer.StartArray ();

  for (octave_idx_type i = 0; i < numel; ++i)
    {
      writer.StartObject ();
      for (octave_idx_type k = 0; k < keys.numel (); ++k)
        {
          writer.Key (keys(k).c_str ());
          encode (writer, struct_array(i).getfield (keys(k)), ConvertInfAndNaN);
        }
      writer.EndObject ();
    }

  if (is_array)
    writer.EndArray ();
}

//! Encodes a Cell Octave value into a JSON array
//!
//! @param writer RapidJSON's writer that is responsible for generating JSON.
//! @param obj Cell Octave value.
//! @param ConvertInfAndNaN @c bool that converts @c Inf and @c NaN to @c null.
//!
//! @b Example:
//!
//! @code{.cc}
//! octave_value obj (cell ());
//! encode_cell (writer, obj,true);
//! @endcode

template <typename T> void
encode_cell (T& writer, const octave_value& obj, const bool& ConvertInfAndNaN)
{
  Cell cell = obj.cell_value ();

  writer.StartArray ();

  for (octave_idx_type i = 0; i < cell.numel (); ++i)
    encode (writer, cell(i), ConvertInfAndNaN);

  writer.EndArray ();
}

//! Encodes a numeric or logical Octave array into a JSON array
//!
//! @param writer RapidJSON's writer that is responsible for generating JSON.
//! @param obj numeric or logical Octave array.
//! @param ConvertInfAndNaN @c bool that converts @c Inf and @c NaN to @c null.
//! @param original_dims The original dimensions of the array being encoded.
//! @param level The level of recursion for the function.
//! @param is_logical optional @c bool that indicates if the array is logical.
//!
//! @b Example:
//!
//! @code{.cc}
//! octave_value obj (NDArray ());
//! encode_array (writer, obj,true);
//! @endcode

template <typename T> void
encode_array (T& writer, const octave_value& obj, const bool& ConvertInfAndNaN,
              const dim_vector& original_dims, int level = 0,
              bool is_logical = false)
{
  NDArray array = obj.array_value ();
  // is_logical is assigned at level 0.  I think this is better than changing
  // many places in the code, and it makes the function more modular.
  if (level == 0)
    is_logical = obj.islogical ();

  if (array.isempty ())
    {
      writer.StartArray ();
      writer.EndArray ();
    }
  else if (array.isvector ())
    {
      writer.StartArray ();
      for (octave_idx_type i = 0; i < array.numel (); ++i)
        {
          if (is_logical)
            encode_numeric (writer, bool (array(i)), ConvertInfAndNaN);
          else
            encode_numeric (writer, array(i), ConvertInfAndNaN);
        }
      writer.EndArray ();
    }
  else
    {
      octave_idx_type idx;
      octave_idx_type ndims = array.ndims ();
      dim_vector dims = array.dims ();

      // In this case, we already have a vector. So,  we transform it to 2-D
      // vector in order to be detected by "isvector" in the recursive call
      if (dims.num_ones () == ndims - 1)
        {
          // Handle the special case when the input is a vector with more than
          // 2 dimensions (e.g. ones ([1 1 1 1 1 6])). In this case, we don't
          // add dimension brackets and treat it as if it is a vector
          if (level != 0)
            // Place an opening and a closing bracket (represents a dimension)
            // for every dimension that equals 1 till we reach the 2-D vector
            for (int i = level; i < ndims - 1; ++i)
              writer.StartArray ();

          encode_array (writer, array.as_row (), ConvertInfAndNaN,
                        original_dims, level + 1, is_logical);

          if (level != 0)
            for (int i = level; i < ndims - 1; ++i)
              writer.EndArray ();
        }
      else
        {
          // We place an opening and a closing bracket for each dimension
          // that equals 1 to preserve the number of dimensions when decoding
          // the array after encoding it.
          if (original_dims (level) == 1)
          {
            writer.StartArray ();
            encode_array (writer, array, ConvertInfAndNaN,
                          original_dims, level + 1, is_logical);
            writer.EndArray ();
          }
          else
            {
              for (idx = 0; idx < ndims; ++idx)
                if (dims(idx) != 1)
                  break;

              // Create the dimensions that will be used to call "num2cell"
              // We called "num2cell" to divide the array to smaller sub-arrays
              // in order to encode it recursively.
              // The recursive encoding is necessary to support encoding of
              // higher-dimensional arrays.
              RowVector conversion_dims;
              conversion_dims.resize (ndims - 1);
              for (octave_idx_type i = 0; i < idx; ++i)
                conversion_dims(i) = i + 1;
              for (octave_idx_type i = idx ; i < ndims - 1; ++i)
                conversion_dims(i) = i + 2;

              octave_value_list args (obj);
              args.append (conversion_dims);

              Cell sub_arrays = Fnum2cell (args)(0).cell_value ();

              writer.StartArray ();

              for (octave_idx_type i = 0; i < sub_arrays.numel (); ++i)
                encode_array (writer, sub_arrays(i), ConvertInfAndNaN,
                              original_dims, level + 1, is_logical);

              writer.EndArray ();
            }
        }
    }
}

//! Encodes any Octave object. This function only serves as an interface
//! by choosing which function to call from the previous functions.
//!
//! @param writer RapidJSON's writer that is responsible for generating JSON.
//! @param obj any @ref octave_value that is supported.
//! @param ConvertInfAndNaN @c bool that converts @c Inf and @c NaN to @c null.
//!
//! @b Example:
//!
//! @code{.cc}
//! octave_value obj (true);
//! encode (writer, obj,true);
//! @endcode

template <typename T> void
encode (T& writer, const octave_value& obj, const bool& ConvertInfAndNaN)
{
  if (obj.is_real_scalar ())
    encode_numeric (writer, obj, ConvertInfAndNaN);
  // As I checked for scalars, this will detect numeric & logical arrays
  else if (obj.isnumeric () || obj.islogical ())
    encode_array (writer, obj, ConvertInfAndNaN, obj.dims ());
  else if (obj.is_string ())
    encode_string (writer, obj, obj.dims ());
  else if (obj.isstruct ())
    encode_struct (writer, obj, ConvertInfAndNaN);
  else if (obj.iscell ())
    encode_cell (writer, obj, ConvertInfAndNaN);
  else if (obj.class_name () == "containers.Map")
    // To extract the data in containers.Map, convert it to a struct.
    // The struct will have a "map" field whose value is a struct that
    // contains the desired data.
    // To avoid warnings due to that conversion, disable the
    // "Octave:classdef-to-struct" warning and re-enable it.
    {
      octave::unwind_action restore_warning_state
        ([] (const octave_value_list& old_warning_state)
         {
           set_warning_state (old_warning_state);
         }, set_warning_state ("Octave:classdef-to-struct", "off"));

      encode_struct (writer, obj.scalar_map_value ().getfield ("map"),
                     ConvertInfAndNaN);
    }
  else if (obj.isobject ())
    {
      octave::unwind_action restore_warning_state
        ([] (const octave_value_list& old_warning_state)
         {
           set_warning_state (old_warning_state);
         }, set_warning_state ("Octave:classdef-to-struct", "off"));

      encode_struct (writer, obj.scalar_map_value (), ConvertInfAndNaN);
    }
  else
    error ("jsonencode: unsupported type");
}

#endif

DEFUN_DLD (jsonencode, args, ,
           "-*- texinfo -*-                                                  \n\
@deftypefn  {} {@var{JSON_txt} =} jsonencode (@var{object})                  \n\
@deftypefnx {} {@var{JSON_txt} =} jsonencode (@dots{}, \"ConvertInfAndNaN\", @var{TF}) \n\
@deftypefnx {} {@var{JSON_txt} =} jsonencode (@dots{}, \"PrettyPrint\", @var{TF}) \n\
                                                                             \n\
Encode Octave data types into JSON text.                                     \n\
                                                                             \n\
The input @var{object} is an Octave variable to encode.                      \n\
                                                                             \n\
The output @var{JSON_txt} is the JSON text that contains the result of encoding \n\
@var{object}.                                                                \n\
                                                                             \n\
If the value of the option @qcode{\"ConvertInfAndNaN\"} is true then @code{NaN}, \n\
@code{NA}, @code{-Inf}, and @code{Inf} values will be converted to           \n\
@qcode{\"null\"} in the output.  If it is false then they will remain as their \n\
original values.  The default value for this option is true.                 \n\
                                                                             \n\
If the value of the option @qcode{\"PrettyPrint\"} is true, the output text will \n\
have indentations and line feeds.  If it is false, the output will be condensed \n\
and written without whitespace.  The default value for this option is false. \n\
                                                                             \n\
Programming Notes:                                                           \n\
                                                                             \n\
@itemize @bullet                                                             \n\
@item                                                                        \n\
Complex numbers are not supported.                                           \n\
                                                                             \n\
@item                                                                        \n\
classdef objects are first converted to structs and then encoded.            \n\
                                                                             \n\
@item                                                                        \n\
To preserve escape characters (e.g., @qcode{\"@backslashchar{}n\"}), use     \n\
single-quoted strings.                                                       \n\
                                                                             \n\
@item                                                                        \n\
Every character after the null character (@qcode{\"@backslashchar\{}0\"}) in a  \n\
double-quoted string will be dropped during encoding.                        \n\
                                                                             \n\
@item                                                                        \n\
Encoding and decoding an array is not guaranteed to preserve the dimensions  \n\
of the array.  In particular, row vectors will be reshaped to column vectors.\n\
                                                                             \n\
@item                                                                        \n\
Encoding and decoding is not guaranteed to preserve the Octave data type     \n\
because JSON supports fewer data types than Octave.  For example, if you     \n\
encode an @code{int8} and then decode it, you will get a @code{double}.      \n\
@end itemize                                                                 \n\
                                                                             \n\
This table shows the conversions from Octave data types to JSON data types:  \n\
                                                                             \n\
@multitable @columnfractions 0.50 0.50                                       \n\
@headitem Octave data type @tab JSON data type                               \n\
@item logical scalar @tab Boolean                                            \n\
@item logical vector @tab Array of Boolean, reshaped to row vector           \n\
@item logical array  @tab nested Array of Boolean                            \n\
@item numeric scalar @tab Number                                             \n\
@item numeric vector @tab Array of Number, reshaped to row vector            \n\
@item numeric array  @tab nested Array of Number                             \n\
@item @code{NaN}, @code{NA}, @code{Inf}, @code{-Inf}@*                       \n\
when @qcode{\"ConvertInfAndNaN\" = true} @tab @qcode{\"null\"}               \n\
@item @code{NaN}, @code{NA}, @code{Inf}, @code{-Inf}@*                       \n\
when @qcode{\"ConvertInfAndNaN\" = false} @tab @qcode{\"NaN\"}, @qcode{\"NaN\"}, \n\
@qcode{\"Infinity\"}, @qcode{\"-Infinity\"}                                  \n\
@item empty array    @tab @qcode{\"[]\"}                                     \n\
@item character vector @tab String                                           \n\
@item character array @tab Array of String                                   \n\
@item empty character array @tab @qcode{\"\"}                                \n\
@item cell scalar @tab Array                                                 \n\
@item cell vector @tab Array, reshaped to row vector                         \n\
@item cell array @tab Array, flattened to row vector                         \n\
@item struct scalar @tab Object                                              \n\
@item struct vector @tab Array of Object, reshaped to row vector             \n\
@item struct array  @tab nested Array of Object                              \n\
@item classdef object @tab Object                                            \n\
@end multitable                                                              \n\
                                                                             \n\
Examples:                                                                    \n\
                                                                             \n\
@example                                                                     \n\
@group                                                                       \n\
jsonencode ([1, NaN; 3, 4])                                                  \n\
@result\{} [[1,null],[3,4]]                                                  \n\
@end group                                                                   \n\
                                                                             \n\
@group                                                                       \n\
jsonencode ([1, NaN; 3, 4], \"ConvertInfAndNaN\", false)                     \n\
@result\{} [[1,NaN],[3,4]]                                                   \n\
@end group                                                                   \n\
                                                                             \n\
@group                                                                       \n\
## Escape characters inside a single-quoted string                           \n\
jsonencode ('\\0\\a\\b\\t\\n\\v\\f\\r')                                      \n\
@result\{} \"\\\\0\\\\a\\\\b\\\\t\\\\n\\\\v\\\\f\\\\r\"                      \n\
@end group                                                                   \n\
                                                                             \n\
@group                                                                       \n\
## Escape characters inside a double-quoted string                           \n\
jsonencode (\"\\a\\b\\t\\n\\v\\f\\r\")                                       \n\
@result{} \"\\u0007\\b\\t\\n\\u000B\\f\\r\"                                  \n\
@end group                                                                   \n\
                                                                             \n\
@group                                                                       \n\
jsonencode ([true; false], \"PrettyPrint\", true)                            \n\
@result{} ans = [                                                            \n\
     true,                                                                   \n\
     false                                                                   \n\
   ]                                                                         \n\
@end group                                                                   \n\
                                                                             \n\
@group                                                                       \n\
jsonencode (['foo', 'bar'; 'foo', 'bar'])                                    \n\
@result\{} [\"foobar\",\"foobar\"]                                           \n\
@end group                                                                   \n\
                                                                             \n\
@group                                                                       \n\
jsonencode (struct ('a', Inf, 'b', [], 'c', struct ()))                      \n\
@result\{} @{\"a\":null,\"b\":[],\"c\":@{@}@}                                \n\
@end group                                                                   \n\
                                                                             \n\
@group                                                                       \n\
jsonencode (struct ('structarray', struct ('a', @{1; 3@}, 'b', @{2; 4@})))   \n\
@result\{} @{\"structarray\":[@{\"a\":1,\"b\":2@},@{\"a\":3,\"b\":4@}]@}     \n\
@end group                                                                   \n\
                                                                             \n\
@group                                                                       \n\
jsonencode (@{'foo'; 'bar'; @{'foo'; 'bar'@}@})                              \n\
@result\{} [\"foo\",\"bar\",[\"foo\",\"bar\"]]                               \n\
@end group                                                                   \n\
                                                                             \n\
@group                                                                       \n\
jsonencode (containers.Map(@{'foo'; 'bar'; 'baz'@}, [1, 2, 3]))              \n\
@result\{} @{\"bar\":2,\"baz\":3,\"foo\":1@}                                 \n\
@end group                                                                   \n\
@end example                                                                 \n\
                                                                             \n\
@seealso{jsondecode}                                                         \n\
@end deftypefn")
{
#if defined (HAVE_RAPIDJSON)

  int nargin = args.length ();
  // jsonencode has two options 'ConvertInfAndNaN' and 'PrettyPrint'
  if (nargin != 1 && nargin != 3 && nargin != 5)
    print_usage ();

  // Initialize options with their default values
  bool ConvertInfAndNaN = true;
  bool PrettyPrint = false;

  for (octave_idx_type i = 1; i < nargin; ++i)
    {
      if (! args(i).is_string ())
        error ("jsonencode: option must be a string");
      if (! args(i+1).is_bool_scalar ())
        error ("jsonencode: option value must be a logical scalar");

      std::string option_name = args(i++).string_value ();
      if (octave::string::strcmpi (option_name, "ConvertInfAndNaN"))
        ConvertInfAndNaN = args(i).bool_value ();
      else if (octave::string::strcmpi (option_name, "PrettyPrint"))
        PrettyPrint = args(i).bool_value ();
      else
        error ("jsonencode: "
               R"(Valid options are "ConvertInfAndNaN" and "PrettyPrint")");
    }

# if ! defined (HAVE_RAPIDJSON_PRETTYWRITER)
  if (PrettyPrint)
    {
      warn_disabled_feature ("jsonencode",
                             R"(the "PrettyPrint" option of RapidJSON)");
      PrettyPrint = false;
    }
# endif

  rapidjson::StringBuffer json;
  if (PrettyPrint)
    {
# if defined (HAVE_RAPIDJSON_PRETTYWRITER)
      rapidjson::PrettyWriter<rapidjson::StringBuffer, rapidjson::UTF8<>,
                              rapidjson::UTF8<>, rapidjson::CrtAllocator,
                              rapidjson::kWriteNanAndInfFlag> writer (json);
      writer.SetIndent (' ', 2);
      encode (writer, args(0), ConvertInfAndNaN);
# endif
    }
  else
    {
      rapidjson::Writer<rapidjson::StringBuffer, rapidjson::UTF8<>,
                        rapidjson::UTF8<>, rapidjson::CrtAllocator,
                        rapidjson::kWriteNanAndInfFlag> writer (json);
      encode (writer, args(0), ConvertInfAndNaN);
    }

  return octave_value (json.GetString ());

#else

  octave_unused_parameter (args);

  err_disabled_feature ("jsonencode", "JSON encoding through RapidJSON");

#endif
}

/*
Functional BIST tests are located in test/json/jsonencode_BIST.tst

## Input validation tests
%!test
%! fail ("jsonencode ()");
%! fail ("jsonencode (1, 2)");
%! fail ("jsonencode (1, 2, 3, 4)");
%! fail ("jsonencode (1, 2, 3, 4, 5, 6)");
%! fail ("jsonencode (1, 2, true)", "option must be a string");
%! fail ("jsonencode (1, 'string', ones (2,2))", ...
%!       "option value must be a logical scalar");
%! fail ("jsonencode (1, 'foobar', true)", ...
%!       'Valid options are "ConvertInfAndNaN"');

*/
