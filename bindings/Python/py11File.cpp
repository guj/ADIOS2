/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * py11File.cpp
 *
 *  Created on: Mar 6, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "py11File.h"

#include <algorithm>
#include <iostream>

#include "adios2/ADIOSMPI.h"
#include "adios2/ADIOSMacros.h"
#include "adios2/helper/adiosFunctions.h"

#include "py11types.h"

namespace adios2
{
namespace py11
{

File::File(const std::string &name, const std::string mode, MPI_Comm comm,
           const std::string engineType, const Params &parameters,
           const vParams &transportParameters)
: m_Name(name), m_Mode(mode)
{
    if (mode == "r")
    {
        m_Stream = std::make_shared<core::Stream>(name, adios2::Mode::Read,
                                                  comm, engineType, parameters,
                                                  transportParameters);
    }
    else if (mode == "w")
    {
        m_Stream = std::make_shared<core::Stream>(name, adios2::Mode::Write,
                                                  comm, engineType, parameters,
                                                  transportParameters);
    }
    else if (mode == "a")
    {
        m_Stream = std::make_shared<core::Stream>(name, adios2::Mode::Append,
                                                  comm, engineType, parameters,
                                                  transportParameters);
    }
    else
    {
        throw std::invalid_argument(
            "ERROR: adios2 mode " + mode + " for file " + name +
            " not supported, only \"r\", \"w\" and \"a\" (read, write, append) "
            "are valid modes, in call to open\n");
    }
    m_IsClosed = false;
}

File::File(const std::string &name, const std::string mode, MPI_Comm comm,
           const std::string configFile, const std::string ioInConfigFile)
: m_Name(name), m_Mode(mode)
{
    if (mode == "r")
    {
        m_Stream = std::make_shared<core::Stream>(
            name, adios2::Mode::Read, comm, configFile, ioInConfigFile);
    }
    else if (mode == "w")
    {
        m_Stream = std::make_shared<core::Stream>(
            name, adios2::Mode::Write, comm, configFile, ioInConfigFile);
    }
    else if (mode == "a")
    {
        m_Stream = std::make_shared<core::Stream>(
            name, adios2::Mode::Append, comm, configFile, ioInConfigFile);
    }
    else
    {
        throw std::invalid_argument("ERROR: adios2 mode " + mode +
                                    " for file " + name +
                                    " not supported, in call to open\n");
    }
    m_IsClosed = false;
}

File::File(const std::string &name, const std::string mode,
           const std::string engineType, const Params &parameters,
           const vParams &transportParameters)
: File(name, mode, MPI_COMM_SELF, engineType, parameters, transportParameters)
{
}

File::File(const std::string &name, const std::string mode,
           const std::string configFile, const std::string ioInConfigFile)
: File(name, mode, MPI_COMM_SELF, configFile, ioInConfigFile)
{
}

bool File::eof() const
{
    bool eof = false;

    if (m_Stream->m_Status == StepStatus::EndOfStream)
    {
        eof = true;
    }

    return eof;
}

std::map<std::string, adios2::Params> File::GetAvailableVariables() noexcept
{
    return m_Stream->m_IO->GetAvailableVariables();
}

void File::Write(const std::string &name, const pybind11::array &array,
                 const Dims &shape, const Dims &start, const Dims &count,
                 const bool endl)
{
    if (false)
    {
    }
#define declare_type(T)                                                        \
    else if (pybind11::isinstance<                                             \
                 pybind11::array_t<T, pybind11::array::c_style>>(array))       \
    {                                                                          \
        m_Stream->Write(name, reinterpret_cast<const T *>(array.data()),       \
                        shape, start, count, endl);                            \
    }
    ADIOS2_FOREACH_NUMPY_TYPE_1ARG(declare_type)
#undef declare_type
    else
    {
        throw std::invalid_argument(
            "ERROR: adios2 file write variable " + name +
            ", either numpy type is not supported or is"
            "c_style memory contiguous, in call to write\n");
    }
}

void File::Write(const std::string &name, const pybind11::array &array,
                 const bool endl)
{
    Write(name, array, {}, {}, {}, endl);
}

void File::Write(const std::string &name, const std::string &stringValue,
                 const bool endl)
{
    m_Stream->Write(name, stringValue, endl);
}

std::string File::ReadString(const std::string &name, const bool endl)
{
    return m_Stream->Read<std::string>(name, endl).front();
}

std::string File::ReadString(const std::string &name, const size_t step)
{
    std::string value;
    m_Stream->Read<std::string>(name, &value, Box<size_t>(step, 1));
    return value;
}

pybind11::array File::Read(const std::string &name, const bool endl)
{
    const std::string type = m_Stream->m_IO->InquireVariableType(name);

    if (type == "string")
    {
        const std::string value =
            m_Stream->Read<std::string>(name, endl).front();
        pybind11::array pyArray(pybind11::dtype::of<char>(),
                                Dims{value.size()});
        char *pyPtr =
            reinterpret_cast<char *>(const_cast<void *>(pyArray.data()));
        std::copy(value.begin(), value.end(), pyPtr);
        return pyArray;
    }
#define declare_type(T)                                                        \
    else if (type == helper::GetType<T>())                                     \
    {                                                                          \
        core::Variable<T> &variable =                                          \
            *m_Stream->m_IO->InquireVariable<T>(name);                         \
        Dims pyCount;                                                          \
        if (variable.m_SingleValue)                                            \
        {                                                                      \
            pyCount = {1};                                                     \
            pybind11::array pyArray(pybind11::dtype::of<T>(), pyCount);        \
            m_Stream->Read<T>(name, reinterpret_cast<T *>(                     \
                                        const_cast<void *>(pyArray.data())),   \
                              endl);                                           \
            return pyArray;                                                    \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            const Dims zerosStart(variable.m_Shape.size(), 0);                 \
            return Read(name, zerosStart, variable.m_Shape, endl);             \
        }                                                                      \
    }
    ADIOS2_FOREACH_NUMPY_TYPE_1ARG(declare_type)
#undef declare_type
    else
    {
        throw std::invalid_argument(
            "ERROR: adios2 file read variable " + name +
            ", type can't be mapped to a numpy type, in call to read\n");
    }
    return pybind11::array();
}

pybind11::array File::Read(const std::string &name, const Dims &selectionStart,
                           const Dims &selectionCount, const bool endl)
{
    const std::string type = m_Stream->m_IO->InquireVariableType(name);

    if (type.empty())
    {
    }
#define declare_type(T)                                                        \
    else if (type == helper::GetType<T>())                                     \
    {                                                                          \
        pybind11::array pyArray(pybind11::dtype::of<T>(), selectionCount);     \
        m_Stream->Read<T>(                                                     \
            name, reinterpret_cast<T *>(const_cast<void *>(pyArray.data())),   \
            Box<Dims>(selectionStart, selectionCount), endl);                  \
        return pyArray;                                                        \
    }
    ADIOS2_FOREACH_NUMPY_TYPE_1ARG(declare_type)
#undef declare_type

    throw std::invalid_argument(
        "ERROR: adios2 file read variable " + name +
        ", type can't be mapped to a numpy type, in call to read\n");
}

pybind11::array File::Read(const std::string &name, const Dims &selectionStart,
                           const Dims &selectionCount,
                           const size_t stepSelectionStart,
                           const size_t stepSelectionCount)
{
    // shape of the returned numpy array
    Dims shapePy(selectionCount.size() + 1);
    shapePy[0] = stepSelectionCount;
    for (auto i = 1; i < shapePy.size(); ++i)
    {
        shapePy[i] = selectionCount[i - 1];
    }

    const std::string type = m_Stream->m_IO->InquireVariableType(name);

    if (type.empty())
    {
    }
#define declare_type(T)                                                        \
    else if (type == helper::GetType<T>())                                     \
    {                                                                          \
        pybind11::array pyArray(pybind11::dtype::of<T>(), shapePy);            \
        m_Stream->Read<T>(                                                     \
            name, reinterpret_cast<T *>(const_cast<void *>(pyArray.data())),   \
            Box<Dims>(selectionStart, selectionCount),                         \
            Box<size_t>(stepSelectionStart, stepSelectionCount));              \
        return pyArray;                                                        \
    }
    ADIOS2_FOREACH_NUMPY_TYPE_1ARG(declare_type)
#undef declare_type
    else
    {
        throw std::invalid_argument(
            "ERROR: adios2 file read variable " + name +
            ", type can't be mapped to a numpy type, in call to read\n");
    }
    return pybind11::array();
}

void File::Close()
{
    m_Stream->Close();
    m_IsClosed = true;
}

bool File::IsClosed() const noexcept { return m_IsClosed; }

size_t File::CurrentStep() const { return m_Stream->m_Engine->CurrentStep(); };
} // end namespace py11
} // end namespace adios2
