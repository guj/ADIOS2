/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * InSituMPIWriter.tcc implementation of template functions with known type
 *
 *  Created on: Dec 18, 2017
 *      Author: Norbert Podhorszki pnorbert@ornl.gov
 */
#ifndef ADIOS2_ENGINE_INSITUMPIWRITER_TCC_
#define ADIOS2_ENGINE_INSITUMPIWRITER_TCC_

#include "InSituMPIWriter.h"

#include <iostream>

namespace adios2
{

template <class T>
void InSituMPIWriter::PutSyncCommon(Variable<T> &variable, const T *values)
{
    // set variable
    variable.SetData(values);
    if (m_Verbosity == 5)
    {
        std::cout << "InSituMPI Writer PutSync(" << variable.m_Name << ")\n";
    }
}

template <class T>
void InSituMPIWriter::PutDeferredCommon(Variable<T> &variable, const T *values)
{
    variable.SetData(values);
    if (m_Verbosity == 5)
    {
        std::cout << "InSituMPI Writer PutDeferred(" << variable.m_Name
                  << ")\n";
    }
}

} // end namespace adios2

#endif /* ADIOS2_ENGINE_INSITUMPIWRITER_TCC_ */
