/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 */
#include <cstdint>
#include <string>

#include <iostream>
#include <stdexcept>

#include <adios2.h>

#include <gtest/gtest.h>

#include "../SmallTestData.h"

class BPWriteReadAttributeTestADIOS2 : public ::testing::Test
{
public:
    BPWriteReadAttributeTestADIOS2() = default;

    SmallTestData m_TestData;
};

// ADIOS2 write, read for single value attributes
TEST_F(BPWriteReadAttributeTestADIOS2, ADIOS2BPWriteReadSingleTypes)
{
    const std::string fName = "foo" + std::string(&adios2::PathSeparator, 1) +
                              "ADIOS2BPWriteAttributeReadSingleTypes.bp";

    const std::string zero = std::to_string(0);
    const std::string s1_Single = std::string("s1_Single_") + zero;
    const std::string i8_Single = std::string("i8_Single_") + zero;
    const std::string i16_Single = std::string("i16_Single_") + zero;
    const std::string i32_Single = std::string("i32_Single_") + zero;
    const std::string i64_Single = std::string("i64_Single_") + zero;
    const std::string u8_Single = std::string("u8_Single_") + zero;
    const std::string u16_Single = std::string("u16_Single_") + zero;
    const std::string u32_Single = std::string("u32_Single_") + zero;
    const std::string u64_Single = std::string("u64_Single_") + zero;
    const std::string r32_Single = std::string("r32_Single_") + zero;
    const std::string r64_Single = std::string("r64_Single_") + zero;

    // When collective meta generation has landed, use
    // generateNewSmallTestData(m_TestData, 0, mpiRank, mpiSize);
    // Generate current testing data
    SmallTestData currentTestData =
        generateNewSmallTestData(m_TestData, 0, 0, 0);

// Write test data using BP
#ifdef ADIOS2_HAVE_MPI
    adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
#else
    adios2::ADIOS adios(true);
#endif
    {
        adios2::IO io = adios.DeclareIO("TestIO");

        // Declare Single Value Attributes
        io.DefineAttribute<std::string>(s1_Single, currentTestData.S1);
        io.DefineAttribute<int8_t>(i8_Single, currentTestData.I8.front());
        io.DefineAttribute<int16_t>(i16_Single, currentTestData.I16.front());
        io.DefineAttribute<int32_t>(i32_Single, currentTestData.I32.front());
        io.DefineAttribute<int64_t>(i64_Single, currentTestData.I64.front());

        io.DefineAttribute<uint8_t>(u8_Single, currentTestData.U8.front());
        io.DefineAttribute<uint16_t>(u16_Single, currentTestData.U16.front());
        io.DefineAttribute<uint32_t>(u32_Single, currentTestData.U32.front());
        io.DefineAttribute<uint64_t>(u64_Single, currentTestData.U64.front());

        io.DefineAttribute<float>(r32_Single, currentTestData.R32.front());
        io.DefineAttribute<double>(r64_Single, currentTestData.R64.front());

        io.SetEngine("BPFile");
        io.AddTransport("File");

        adios2::Engine engine = io.Open(fName, adios2::Mode::Write);
        // only attributes are written
        engine.Close();
    }

    {
        adios2::IO ioRead = adios.DeclareIO("ioRead");
        // ioRead.SetEngine("ADIOS1");
        // ioRead.AddTransport("File");
        // ioRead.SetParameter("OpenAsFile", "true");

        adios2::Engine bpRead = ioRead.Open(fName, adios2::Mode::Read);

        auto attr_s1 = ioRead.InquireAttribute<std::string>(s1_Single);
        auto attr_i8 = ioRead.InquireAttribute<int8_t>(i8_Single);
        auto attr_i16 = ioRead.InquireAttribute<int16_t>(i16_Single);
        auto attr_i32 = ioRead.InquireAttribute<int32_t>(i32_Single);
        auto attr_i64 = ioRead.InquireAttribute<int64_t>(i64_Single);

        auto attr_u8 = ioRead.InquireAttribute<uint8_t>(u8_Single);
        auto attr_u16 = ioRead.InquireAttribute<uint16_t>(u16_Single);
        auto attr_u32 = ioRead.InquireAttribute<uint32_t>(u32_Single);
        auto attr_u64 = ioRead.InquireAttribute<uint64_t>(u64_Single);

        auto attr_r32 = ioRead.InquireAttribute<float>(r32_Single);
        auto attr_r64 = ioRead.InquireAttribute<double>(r64_Single);

        EXPECT_TRUE(attr_s1);
        ASSERT_EQ(attr_s1.Name(), s1_Single);
        ASSERT_EQ(attr_s1.Data().size() == 1, true);
        ASSERT_EQ(attr_s1.Type(), "string");
        ASSERT_EQ(attr_s1.Data().front(), currentTestData.S1);

        EXPECT_TRUE(attr_i8);
        ASSERT_EQ(attr_i8.Name(), i8_Single);
        ASSERT_EQ(attr_i8.Data().size() == 1, true);
        ASSERT_EQ(attr_i8.Type(), "signed char");
        ASSERT_EQ(attr_i8.Data().front(), currentTestData.I8.front());

        EXPECT_TRUE(attr_i16);
        ASSERT_EQ(attr_i16.Name(), i16_Single);
        ASSERT_EQ(attr_i16.Data().size() == 1, true);
        ASSERT_EQ(attr_i16.Type(), "short");
        ASSERT_EQ(attr_i16.Data().front(), currentTestData.I16.front());

        EXPECT_TRUE(attr_i32);
        ASSERT_EQ(attr_i32.Name(), i32_Single);
        ASSERT_EQ(attr_i32.Data().size() == 1, true);
        ASSERT_EQ(attr_i32.Type(), "int");
        ASSERT_EQ(attr_i32.Data().front(), currentTestData.I32.front());

        EXPECT_TRUE(attr_i64);
        ASSERT_EQ(attr_i64.Name(), i64_Single);
        ASSERT_EQ(attr_i64.Data().size() == 1, true);
#if defined(_WIN32) || defined(__APPLE__)
        ASSERT_EQ(attr_i64.Type(), "long long int");
#else
        ASSERT_EQ(attr_i64.Type(), "long int");
#endif
        ASSERT_EQ(attr_i64.Data().front(), currentTestData.I64.front());

        EXPECT_TRUE(attr_u8);
        ASSERT_EQ(attr_u8.Name(), u8_Single);
        ASSERT_EQ(attr_u8.Data().size() == 1, true);
        ASSERT_EQ(attr_u8.Type(), "unsigned char");
        ASSERT_EQ(attr_u8.Data().front(), currentTestData.U8.front());

        EXPECT_TRUE(attr_u16);
        ASSERT_EQ(attr_u16.Name(), u16_Single);
        ASSERT_EQ(attr_u16.Data().size() == 1, true);
        ASSERT_EQ(attr_u16.Type(), "unsigned short");
        ASSERT_EQ(attr_u16.Data().front(), currentTestData.U16.front());

        EXPECT_TRUE(attr_u32);
        ASSERT_EQ(attr_u32.Name(), u32_Single);
        ASSERT_EQ(attr_u32.Data().size() == 1, true);
        ASSERT_EQ(attr_u32.Type(), "unsigned int");
        ASSERT_EQ(attr_u32.Data().front(), currentTestData.U32.front());

        EXPECT_TRUE(attr_u64);
        ASSERT_EQ(attr_u64.Name(), u64_Single);
        ASSERT_EQ(attr_u64.Data().size() == 1, true);
#if defined(_WIN32) || defined(__APPLE__)
        ASSERT_EQ(attr_u64.Type(), "unsigned long long int");
#else
        ASSERT_EQ(attr_u64.Type(), "unsigned long int");
#endif
        ASSERT_EQ(attr_u64.Data().front(), currentTestData.U64.front());

        EXPECT_TRUE(attr_r32);
        ASSERT_EQ(attr_r32.Name(), r32_Single);
        ASSERT_EQ(attr_r32.Data().size() == 1, true);
        ASSERT_EQ(attr_r32.Type(), "float");
        ASSERT_EQ(attr_r32.Data().front(), currentTestData.R32.front());

        EXPECT_TRUE(attr_r64);
        ASSERT_EQ(attr_r64.Name(), r64_Single);
        ASSERT_EQ(attr_r64.Data().size() == 1, true);
        ASSERT_EQ(attr_r64.Type(), "double");
        ASSERT_EQ(attr_r64.Data().front(), currentTestData.R64.front());

        bpRead.Close();
    }
}

// ADIOS2 write read for array attributes
TEST_F(BPWriteReadAttributeTestADIOS2, ADIOS2BPWriteReadArrayTypes)
{
    const std::string fName = "foo" + std::string(&adios2::PathSeparator, 1) +
                              "ADIOS2BPWriteAttributeReadArrayTypes.bp";

    int mpiRank = 0, mpiSize = 1;
#ifdef ADIOS2_HAVE_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
#endif

    const std::string zero = std::to_string(0);
    const std::string s1_Array = std::string("s1_Array_") + zero;
    const std::string i8_Array = std::string("i8_Array_") + zero;
    const std::string i16_Array = std::string("i16_Array_") + zero;
    const std::string i32_Array = std::string("i32_Array_") + zero;
    const std::string i64_Array = std::string("i64_Array_") + zero;
    const std::string u8_Array = std::string("u8_Array_") + zero;
    const std::string u16_Array = std::string("u16_Array_") + zero;
    const std::string u32_Array = std::string("u32_Array_") + zero;
    const std::string u64_Array = std::string("u64_Array_") + zero;
    const std::string r32_Array = std::string("r32_Array_") + zero;
    const std::string r64_Array = std::string("r64_Array_") + zero;

    // When collective meta generation has landed, use
    // generateNewSmallTestData(m_TestData, 0, mpiRank, mpiSize);
    // Generate current testing data
    SmallTestData currentTestData =
        generateNewSmallTestData(m_TestData, 0, 0, 0);

// Write test data using BP
#ifdef ADIOS2_HAVE_MPI
    adios2::ADIOS adios(MPI_COMM_WORLD, adios2::DebugON);
#else
    adios2::ADIOS adios(true);
#endif
    {
        adios2::IO io = adios.DeclareIO("TestIO");

        // Declare Single Value Attributes
        io.DefineAttribute<std::string>(s1_Array, currentTestData.S3.data(),
                                        currentTestData.S3.size());

        io.DefineAttribute<int8_t>(i8_Array, currentTestData.I8.data(),
                                   currentTestData.I8.size());
        io.DefineAttribute<int16_t>(i16_Array, currentTestData.I16.data(),
                                    currentTestData.I16.size());
        io.DefineAttribute<int32_t>(i32_Array, currentTestData.I32.data(),
                                    currentTestData.I32.size());
        io.DefineAttribute<int64_t>(i64_Array, currentTestData.I64.data(),
                                    currentTestData.I64.size());

        io.DefineAttribute<uint8_t>(u8_Array, currentTestData.U8.data(),
                                    currentTestData.U8.size());
        io.DefineAttribute<uint16_t>(u16_Array, currentTestData.U16.data(),
                                     currentTestData.U16.size());
        io.DefineAttribute<uint32_t>(u32_Array, currentTestData.U32.data(),
                                     currentTestData.U32.size());
        io.DefineAttribute<uint64_t>(u64_Array, currentTestData.U64.data(),
                                     currentTestData.U64.size());

        io.DefineAttribute<float>(r32_Array, currentTestData.R32.data(),
                                  currentTestData.R32.size());
        io.DefineAttribute<double>(r64_Array, currentTestData.R64.data(),
                                   currentTestData.R64.size());

        io.SetEngine("BPFile");
        io.AddTransport("file");

        adios2::Engine engine = io.Open(fName, adios2::Mode::Write);
        // only attributes are written
        engine.Close();
    }

    {
        adios2::IO ioRead = adios.DeclareIO("ioRead");

        adios2::Engine bpRead = ioRead.Open(fName, adios2::Mode::Read);

        auto attr_s1 = ioRead.InquireAttribute<std::string>(s1_Array);

        auto attr_i8 = ioRead.InquireAttribute<int8_t>(i8_Array);
        auto attr_i16 = ioRead.InquireAttribute<int16_t>(i16_Array);
        auto attr_i32 = ioRead.InquireAttribute<int32_t>(i32_Array);
        auto attr_i64 = ioRead.InquireAttribute<int64_t>(i64_Array);

        auto attr_u8 = ioRead.InquireAttribute<uint8_t>(u8_Array);
        auto attr_u16 = ioRead.InquireAttribute<uint16_t>(u16_Array);
        auto attr_u32 = ioRead.InquireAttribute<uint32_t>(u32_Array);
        auto attr_u64 = ioRead.InquireAttribute<uint64_t>(u64_Array);

        auto attr_r32 = ioRead.InquireAttribute<float>(r32_Array);
        auto attr_r64 = ioRead.InquireAttribute<double>(r64_Array);

        EXPECT_TRUE(attr_s1);
        ASSERT_EQ(attr_s1.Name(), s1_Array);
        ASSERT_EQ(attr_s1.Data().size() == 1, false);
        ASSERT_EQ(attr_s1.Type(), "string");

        EXPECT_TRUE(attr_i8);
        ASSERT_EQ(attr_i8.Name(), i8_Array);
        ASSERT_EQ(attr_i8.Data().size() == 1, false);
        ASSERT_EQ(attr_i8.Type(), "signed char");

        EXPECT_TRUE(attr_i16);
        ASSERT_EQ(attr_i16.Name(), i16_Array);
        ASSERT_EQ(attr_i16.Data().size() == 1, false);
        ASSERT_EQ(attr_i16.Type(), "short");

        EXPECT_TRUE(attr_i32);
        ASSERT_EQ(attr_i32.Name(), i32_Array);
        ASSERT_EQ(attr_i32.Data().size() == 1, false);
        ASSERT_EQ(attr_i32.Type(), "int");

        EXPECT_TRUE(attr_i64);
        ASSERT_EQ(attr_i64.Name(), i64_Array);
        ASSERT_EQ(attr_i64.Data().size() == 1, false);
#if defined(_WIN32) || defined(__APPLE__)
        ASSERT_EQ(attr_i64.Type(), "long long int");
#else
        ASSERT_EQ(attr_i64.Type(), "long int");
#endif

        EXPECT_TRUE(attr_u8);
        ASSERT_EQ(attr_u8.Name(), u8_Array);
        ASSERT_EQ(attr_u8.Data().size() == 1, false);
        ASSERT_EQ(attr_u8.Type(), "unsigned char");

        EXPECT_TRUE(attr_u16);
        ASSERT_EQ(attr_u16.Name(), u16_Array);
        ASSERT_EQ(attr_u16.Data().size() == 1, false);
        ASSERT_EQ(attr_u16.Type(), "unsigned short");

        EXPECT_TRUE(attr_u32);
        ASSERT_EQ(attr_u32.Name(), u32_Array);
        ASSERT_EQ(attr_u32.Data().size() == 1, false);
        ASSERT_EQ(attr_u32.Type(), "unsigned int");

        EXPECT_TRUE(attr_u64);
        ASSERT_EQ(attr_u64.Name(), u64_Array);
        ASSERT_EQ(attr_u64.Data().size() == 1, false);
#if defined(_WIN32) || defined(__APPLE__)
        ASSERT_EQ(attr_u64.Type(), "unsigned long long int");
#else
        ASSERT_EQ(attr_u64.Type(), "unsigned long int");
#endif

        EXPECT_TRUE(attr_r32);
        ASSERT_EQ(attr_r32.Name(), r32_Array);
        ASSERT_EQ(attr_r32.Data().size() == 1, false);
        ASSERT_EQ(attr_r32.Type(), "float");

        EXPECT_TRUE(attr_r64);
        ASSERT_EQ(attr_r64.Name(), r64_Array);
        ASSERT_EQ(attr_r64.Data().size() == 1, false);
        ASSERT_EQ(attr_r64.Type(), "double");

        auto I8 = attr_i8.Data();
        auto I16 = attr_i16.Data();
        auto I32 = attr_i32.Data();
        auto I64 = attr_i64.Data();

        auto U8 = attr_u8.Data();
        auto U16 = attr_u16.Data();
        auto U32 = attr_u32.Data();
        auto U64 = attr_u64.Data();

        const size_t Nx = 10;
        for (size_t i = 0; i < Nx; ++i)
        {
            EXPECT_EQ(I8[i], currentTestData.I8[i]);
            EXPECT_EQ(I16[i], currentTestData.I16[i]);
            EXPECT_EQ(I32[i], currentTestData.I32[i]);
            EXPECT_EQ(I64[i], currentTestData.I64[i]);

            EXPECT_EQ(U8[i], currentTestData.U8[i]);
            EXPECT_EQ(U16[i], currentTestData.U16[i]);
            EXPECT_EQ(U32[i], currentTestData.U32[i]);
            EXPECT_EQ(U64[i], currentTestData.U64[i]);
        }

        bpRead.Close();
    }
}

//******************************************************************************
// main
//******************************************************************************

int main(int argc, char **argv)
{
#ifdef ADIOS2_HAVE_MPI
    MPI_Init(nullptr, nullptr);
#endif

    int result;
    ::testing::InitGoogleTest(&argc, argv);
    result = RUN_ALL_TESTS();

#ifdef ADIOS2_HAVE_MPI
    MPI_Finalize();
#endif

    return result;
}
