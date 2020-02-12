#include "../libpika/Pika.h"
#include "gtest/gtest.h"


TEST(BufferTest, BufferInitialSize) {
    pika::Buffer<int> buffer;

    EXPECT_EQ(buffer.GetSize(), 0);
}

TEST(BufferTest, BufferPushAddsElements) {
    pika::Buffer<int> buffer;
    buffer.Push(5);

    EXPECT_EQ(buffer.GetSize(), 1);
    EXPECT_EQ(buffer[0], 5);
}