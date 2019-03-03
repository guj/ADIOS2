#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#
# TestBPWriteRead2D.py
#
#
#  Created on: Mar 3rd, 2019
#      Author: Kai Germaschewski <kai.germaschewski@unh.edu>
#              William F Godoy godoywf@ornl.gov
#

from mpi4py import MPI
import numpy
import adios2

# MPI
comm = MPI.COMM_WORLD
rank = comm.Get_rank()
size = comm.Get_size()

# User data
Nx = 10
Ny = 10

count = [Nx, Ny]
start = [rank * Nx, 0]
shape = [size * Nx, Ny]

temperatures = numpy.empty(count, dtype=numpy.int)

for i in range(0, Nx):
    iGlobal = start[0] + i

    for j in range(0, Ny):
        value = iGlobal * shape[1] + j
        temperatures[i,j] = value

print(temperatures)
# ADIOS portion
adios = adios2.ADIOS(comm)
ioWrite = adios.DeclareIO("ioWriter")

varTemperature = ioWrite.DefineVariable("temperature2D", temperatures, shape,
                                        start, count, adios2.ConstantDims)

obpStream = ioWrite.Open('HeatMap2D_py.bp', adios2.Mode.Write)
obpStream.Put(varTemperature, temperatures)
obpStream.Close()


if rank == 0:
    ioRead = adios.DeclareIO("ioReader")

    ibpStream = ioRead.Open('HeatMap2D_py.bp', adios2.Mode.Read, MPI.COMM_SELF)

    var_inTemperature = ioRead.InquireVariable("temperature2D")

    if var_inTemperature is not None:
        readOffset = [2, 2]
        readSize = [4, 4]

        var_inTemperature.SetSelection([readOffset, readSize])
        inTemperatures = numpy.zeros(readSize, dtype=numpy.int)
        ibpStream.Get(var_inTemperature, inTemperatures, adios2.Mode.Sync)

        print('Incoming temperature map\n', inTemperatures)

    ibpStream.Close()
