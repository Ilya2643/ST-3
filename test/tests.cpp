// Copyright 2021 GHA Test Team

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cstdint>
#include "TimedDoor.h"
#include <stdexcept>
#include <memory>
#include <thread>
#include <chrono>
using ::testing::Return;

class MockTimerClient : public TimerClient {
 public:
  MOCK_METHOD(void, Timeout, (), (override));
};

// Мок-класс для тестирования Door
class MockDoor : public Door {
 public:
  MOCK_METHOD(void, lock, (), (override));
  MOCK_METHOD(void, unlock, (), (override));
  MOCK_METHOD(bool, isDoorOpened, (), (override));
};

// Фикстура для TimedDoor
class TimedDoorTest : public ::testing::Test {
 protected:
  std::unique_ptr<TimedDoor> door;

  void SetUp() override {
    door = std::make_unique<TimedDoor>(2);
  }
};

TEST_F(TimedDoorTest, InitiallyClosed) {
  ASSERT_FALSE(door->isDoorOpened());
}

TEST_F(TimedDoorTest, CanUnlock) {
  door->unlock();
  EXPECT_TRUE(door->isDoorOpened());
}

TEST_F(TimedDoorTest, CanLock) {
  door->unlock();
  door->lock();
  EXPECT_FALSE(door->isDoorOpened());
}

TEST_F(TimedDoorTest, ThrowsIfOpenTooLong) {
  door->unlock();
  std::this_thread::sleep_for(std::chrono::seconds(3));
  EXPECT_THROW(door->throwState(), std::runtime_error);
}

TEST_F(TimedDoorTest, NoExceptionIfClosedInTime) {
  door->unlock();
  std::this_thread::sleep_for(std::chrono::seconds(1));
  door->lock();
  EXPECT_NO_THROW(door->throwState());
}

TEST_F(TimedDoorTest, ReopenResetsTimer) {
  door->unlock();
  std::this_thread::sleep_for(std::chrono::seconds(1));
  door->unlock();  // Перезапуск таймера – поколение увеличивается
  std::this_thread::sleep_for(std::chrono::seconds(1));
  EXPECT_NO_THROW(door->throwState());
}

// Фикстура для Timer
class TimerTest : public ::testing::Test {
 protected:
  Timer timer;
  MockTimerClient mockClient;
};

// Тест 7: Таймер должен вызвать Timeout() через указанное время
TEST_F(TimerTest, TimerTriggersTimeout) {
  EXPECT_CALL(mockClient, Timeout()).Times(1);
  timer.tregister(1, &mockClient);
  std::this_thread::sleep_for(std::chrono::seconds(2));
}

// Фикстура для DoorTimerAdapter
class DoorTimerAdapterTest : public ::testing::Test {
 protected:
  MockDoor mockDoor;
  std::unique_ptr<DoorTimerAdapter> adapter;

  void SetUp() override {
    adapter = std::make_unique<DoorTimerAdapter>(
      reinterpret_cast<TimedDoor&>(mockDoor));
  }
};

TEST_F(DoorTimerAdapterTest, TimeoutTriggersException) {
  EXPECT_CALL(mockDoor, isDoorOpened()).WillOnce(Return(true));
  adapter->Timeout();
}

TEST_F(DoorTimerAdapterTest, NoExceptionIfDoorClosed) {
  EXPECT_CALL(mockDoor, isDoorOpened()).WillOnce(Return(false));
  adapter->Timeout();
}

TEST_F(TimedDoorTest, TimeoutValueCorrect) {
  EXPECT_EQ(door->getTimeOut(), 2);
}
