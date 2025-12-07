/***************************************************************************
* Copyright (c) 2019, Martin Renou                                         *
*                                                                          *
Copyright (c) 2019,
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

****************************************************************************/

#pragma once

#include <string>
#include <vector>

#include "wpi/json.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>

namespace nb = nanobind;

namespace pyjson
{
    using number_unsigned_t = uint64_t;
    using number_integer_t = int64_t;

    inline nb::object from_json(const wpi::json& j)
    {
        if (j.is_null())
        {
            return nb::none();
        }
        else if (j.is_boolean())
        {
            return nb::bool_(j.get<bool>());
        }
        else if (j.is_number_unsigned())
        {
            return nb::int_(j.get<number_unsigned_t>());
        }
        else if (j.is_number_integer())
        {
            return nb::int_(j.get<number_integer_t>());
        }
        else if (j.is_number_float())
        {
            return nb::float_(j.get<double>());
        }
        else if (j.is_string())
        {
            auto s = j.get<std::string>();
            return nb::str(s.data(), s.size());
        }
        else if (j.is_array())
        {
            nb::object obj(nb::steal(PyList_New(j.size())));
            for (std::size_t i = 0; i < j.size(); i++)
            {
                nb::object o = from_json(j[i]);
                NB_LIST_SET_ITEM(obj.ptr(), i, o.ptr());
            }
            return obj;
        }
        else // Object
        {
            nb::dict obj;
            for (wpi::json::const_iterator it = j.cbegin(); it != j.cend(); ++it)
            {
                auto key = it.key();
                obj[nb::str(key.data(), key.size())] = from_json(it.value());
            }
            return obj;
        }
    }

    inline wpi::json to_json(const nb::handle& obj)
    {
        if (obj.ptr() == nullptr || obj.is_none())
        {
            return nullptr;
        }
        if (nb::isinstance<nb::bool_>(obj))
        {
            return nb::cast<bool>(obj);
        }
        if (nb::isinstance<nb::int_>(obj))
        {
            try
            {
                number_integer_t s = nb::cast<number_integer_t>(obj);
                if (nb::int_(s).equal(obj))
                {
                    return s;
                }
            }
            catch (...)
            {
            }
            try
            {
                number_unsigned_t u = nb::cast<number_integer_t>(obj);
                if (nb::int_(u).equal(obj))
                {
                    return u;
                }
            }
            catch (...)
            {
            }
            std::string msg("to_json received an integer out of range for both number_integer_t and number_unsigned_t type: ");
            msg += nb::cast<std::string>(nb::repr(obj));
            throw nb::value_error(msg.c_str());
        }
        if (nb::isinstance<nb::float_>(obj))
        {
            return nb::cast<double>(obj);
        }
        // if (nb::isinstance<nb::bytes>(obj))
        // {
        //     nb::module_ base64 = nb::module::import("base64");
        //     return base64.attr("b64encode")(obj).attr("decode")("utf-8").cast<std::string>();
        // }
        if (nb::isinstance<nb::str>(obj))
        {
            return nb::cast<std::string>(obj);
        }
        if (nb::isinstance<nb::tuple>(obj) || nb::isinstance<nb::list>(obj))
        {
            auto out = wpi::json::array();
            for (const nb::handle value : obj)
            {
                out.push_back(to_json(value));
            }
            return out;
        }
        if (nb::isinstance<nb::dict>(obj))
        {
            auto out = wpi::json::object();
            for (const nb::handle key : obj)
            {
                if (nb::isinstance<nb::str>(key)) {
                    out[nb::cast<std::string>(key)] = to_json(obj[key]);

                } else if (nb::isinstance<nb::int_>(key) || nb::isinstance<nb::float_>(key) ||
                           nb::isinstance<nb::bool_>(key) || nb::isinstance(key, nb::none())) {
                    // only allow the same implicit conversions python allows
                    out[nb::cast<std::string>(nb::str(key))] = to_json(obj[key]);
                } else {
                    std::string msg("JSON keys must be str, int, float, bool, or None, not ");
                    msg += nb::cast<std::string>(nb::repr(key));
                    throw nb::type_error(msg.c_str());
                }
            }
            return out;
        }
        std::string msg("Object of type ");
        msg += nb::cast<std::string>(obj.type().attr("__name__"));
        msg += " is not JSON serializable";
        throw nb::type_error(msg.c_str());
    }
}

// nlohmann_json serializers
namespace wpi
{
    #define MAKE_NLJSON_SERIALIZER_DESERIALIZER(T)         \
    template <>                                            \
    struct adl_serializer<T>                               \
    {                                                      \
        inline static void to_json(json& j, const T& obj)  \
        {                                                  \
            j = pyjson::to_json(obj);                      \
        }                                                  \
                                                           \
        inline static T from_json(const json& j)           \
        {                                                  \
            return nb::steal<T>(pyjson::from_json(j));     \
        }                                                  \
    }

    #define MAKE_NLJSON_SERIALIZER_ONLY(T)                 \
    template <>                                            \
    struct adl_serializer<T>                               \
    {                                                      \
        inline static void to_json(json& j, const T& obj)  \
        {                                                  \
            j = pyjson::to_json(obj);                      \
        }                                                  \
    }

    MAKE_NLJSON_SERIALIZER_DESERIALIZER(nb::object);

    MAKE_NLJSON_SERIALIZER_DESERIALIZER(nb::bool_);
    MAKE_NLJSON_SERIALIZER_DESERIALIZER(nb::int_);
    MAKE_NLJSON_SERIALIZER_DESERIALIZER(nb::float_);
    MAKE_NLJSON_SERIALIZER_DESERIALIZER(nb::str);

    MAKE_NLJSON_SERIALIZER_DESERIALIZER(nb::list);
    MAKE_NLJSON_SERIALIZER_DESERIALIZER(nb::tuple);
    MAKE_NLJSON_SERIALIZER_DESERIALIZER(nb::dict);

    // MAKE_NLJSON_SERIALIZER_ONLY(nb::handle);
    // MAKE_NLJSON_SERIALIZER_ONLY(nb::detail::item_accessor);
    // MAKE_NLJSON_SERIALIZER_ONLY(nb::detail::list_accessor);
    // MAKE_NLJSON_SERIALIZER_ONLY(nb::detail::tuple_accessor);
    // MAKE_NLJSON_SERIALIZER_ONLY(nb::detail::sequence_accessor);
    // MAKE_NLJSON_SERIALIZER_ONLY(nb::detail::str_attr_accessor);
    // MAKE_NLJSON_SERIALIZER_ONLY(nb::detail::obj_attr_accessor);

    #undef MAKE_NLJSON_SERIALIZER
    #undef MAKE_NLJSON_SERIALIZER_ONLY
}

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

template <> struct type_caster<wpi::json>
{
public:
    NB_TYPE_CASTER(wpi::json, const_name("wpiutil.json"));

    bool from_python(handle src, uint8_t flags, cleanup_list *cleanup) noexcept
    {
        try
        {
            value = pyjson::to_json(src);
            return true;
        }
        catch (builtin_exception &e)
        {
            if (e.type() == exception_type::value_error) {
                PyErr_SetString(PyExc_ValueError, e.what());
            } else {
                PyErr_SetString(PyExc_TypeError, e.what());
            }
        }
        catch (python_error &e)
        {
            e.restore();
        }

        return false;
    }

    static handle from_cpp(wpi::json src, rv_policy policy, cleanup_list *cleanup) noexcept
    {
        object obj = pyjson::from_json(src);
        return obj.release();
    }
};

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)

