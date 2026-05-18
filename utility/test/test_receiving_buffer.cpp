#include <gtest/gtest.h>
#include "receiving_buffer.hpp"

TEST(ReceivingBufferTest, InitialState) {
    receiving_buffer<100> buffer;
    
    auto data = buffer.get_data();
    EXPECT_EQ(data.size(), 0);
    
    auto writable = buffer.get_writable();
    EXPECT_EQ(writable.size(), 100);
}

TEST(ReceivingBufferTest, CommitAndGetData) {
    receiving_buffer<100> buffer;
    
    auto writable = buffer.get_writable();
    writable[0] = 0xAA;
    writable[1] = 0xBB;
    
    buffer.commit(2);
    
    auto data = buffer.get_data();
    EXPECT_EQ(data.size(), 2);
    EXPECT_EQ(data[0], 0xAA);
    EXPECT_EQ(data[1], 0xBB);
    
    writable = buffer.get_writable();
    EXPECT_EQ(writable.size(), 98);
}

TEST(ReceivingBufferTest, Consume) {
    receiving_buffer<100> buffer;
    
    buffer.commit(5);
    EXPECT_EQ(buffer.get_data().size(), 5);
    
    buffer.consume(2);
    EXPECT_EQ(buffer.get_data().size(), 3);
    
    buffer.consume(3);
    EXPECT_EQ(buffer.get_data().size(), 0);
}

TEST(ReceivingBufferTest, Reset) {
    receiving_buffer<100> buffer;
    
    buffer.commit(10);
    buffer.consume(4);
    
    buffer.reset();
    
    EXPECT_EQ(buffer.get_data().size(), 0);
    EXPECT_EQ(buffer.get_writable().size(), 100);
}

TEST(ReceivingBufferTest, MoveDataToFront) {
    receiving_buffer<100> buffer;
    
    auto writable = buffer.get_writable();
    writable[0] = 0x00;
    writable[1] = 0x11;
    writable[2] = 0x22;
    writable[3] = 0x33;
    
    buffer.commit(4);
    buffer.consume(2);
    
    // Data is now [0x22, 0x33] at read_index 2
    auto data = buffer.get_data();
    EXPECT_EQ(data.size(), 2);
    EXPECT_EQ(data[0], 0x22);
    EXPECT_EQ(data[1], 0x33);
    
    buffer.move_data_to_front();
    
    data = buffer.get_data();
    EXPECT_EQ(data.size(), 2);
    EXPECT_EQ(data[0], 0x22);
    EXPECT_EQ(data[1], 0x33);
    
    writable = buffer.get_writable();
    EXPECT_EQ(writable.size(), 98);
    
    // Check writing after move
    writable[0] = 0x44;
    buffer.commit(1);
    
    data = buffer.get_data();
    EXPECT_EQ(data.size(), 3);
    EXPECT_EQ(data[2], 0x44);
}
