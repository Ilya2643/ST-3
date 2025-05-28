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

class MockDoor : public Door {
 public:
  MOCK_METHOD(void, lock, (), (override));
  MOCK_METHOD(void, unlock, (), (override));
  MOCK_METHOD(bool, isDoorOpened, (), (override));
};

class TimedDoorTest : public ::testing::Test {
 protected:
  std::unique_ptr<TimedDoor> door;

  void SetUp() override {
    door = std::make_unique<TimedDoor>(3); // Таймаут 3 секунды
  }
};

TEST_F(TimedDoorTest, DoorInitiallyLocked) {
  ASSERT_FALSE(door->isDoorOpened());
}

TEST_F(TimedDoorTest, SuccessfulUnlockOperation) {
  door->unlock();
  EXPECT_TRUE(door->isDoorOpened());
}

TEST_F(TimedDoorTest, SuccessfulLockAfterUnlock) {
  door->unlock();
  door->lock();
  EXPECT_FALSE(door->isDoorOpened());
}

TEST_F(TimedDoorTest, ThrowsAfterTimeoutPeriod) {
  door->unlock();
  std::this_thread::sleep_for(std::chrono::seconds(4));
  EXPECT_THROW(door->throwState(), std::runtime_error);
}

TEST_F(TimedDoorTest, NoExceptionWhenClosedBeforeTimeout) {
  door->unlock();
  std::this_thread::sleep_for(std::chrono::milliseconds(1500));
  door->lock();
  EXPECT_NO_THROW(door->throwState());
}

TEST_F(TimedDoorTest, TimerResetOnReopen) {
  door->unlock();
  std::this_thread::sleep_for(std::chrono::seconds(2));
  door->unlock(); // Сброс таймера
  std::this_thread::sleep_for(std::chrono::seconds(2));
  EXPECT_NO_THROW(door->throwState());
}

TEST_F(TimedDoorTest, CorrectTimeoutValueInitialization) {
  TimedDoor customDoor(5);
  EXPECT_EQ(customDoor.getTimeOut(), 5);
}

TEST_F(TimedDoorTest, MultipleLockUnlockCycles) {
  for (int i = 0; i < 3; ++i) {
    door->unlock();
    EXPECT_TRUE(door->isDoorOpened());
    door->lock();
    EXPECT_FALSE(door->isDoorOpened());
  }
}

TEST_F(TimedDoorTest, PartialTimeoutPeriod) {
  door->unlock();
  std::this_thread::sleep_for(std::chrono::milliseconds(2500));
  EXPECT_NO_THROW(door->throwState());
}

class TimerTest : public ::testing::Test {
 protected:
  Timer timer;
  MockTimerClient mockClient;
};

TEST_F(TimerTest, TriggersSingleTimeout) {
  EXPECT_CALL(mockClient, Timeout()).Times(1);
  timer.tregister(1500, &mockClient);
  std::this_thread::sleep_for(std::chrono::milliseconds(2000));
}

TEST_F(TimerTest, NoTimeoutWhenUnregistered) {
  EXPECT_CALL(mockClient, Timeout()).Times(0);
  std::this_thread::sleep_for(std::chrono::milliseconds(2000));
}

TEST_F(TimerTest, MultipleClientRegistration) {
  MockTimerClient client2;
  EXPECT_CALL(mockClient, Timeout()).Times(1);
  EXPECT_CALL(client2, Timeout()).Times(1);
  timer.tregister(1000, &mockClient);
  timer.tregister(1000, &client2);
  std::this_thread::sleep_for(std::chrono::seconds(2));
}

class DoorTimerAdapterTest : public ::testing::Test {
 protected:
  MockDoor mockDoor;
  std::unique_ptr<DoorTimerAdapter> adapter;

  void SetUp() override {
    adapter = std::make_unique<DoorTimerAdapter>(
      reinterpret_cast<TimedDoor&>(mockDoor));
  }
};

TEST_F(DoorTimerAdapterTest, ThrowsExceptionWhenDoorOpen) {
  EXPECT_CALL(mockDoor, isDoorOpened()).WillOnce(Return(true));
  EXPECT_THROW(adapter->Timeout(), std::runtime_error);
}

TEST_F(DoorTimerAdapterTest, NoActionWhenDoorClosed) {
  EXPECT_CALL(mockDoor, isDoorOpened()).WillOnce(Return(false));
  EXPECT_NO_THROW(adapter->Timeout());
}

TEST_F(DoorTimerAdapterTest, MultipleTimeoutCalls) {
  EXPECT_CALL(mockDoor, isDoorOpened())
    .Times(2)
    .WillOnce(Return(true))
    .WillOnce(Return(false));
  adapter->Timeout();
  adapter->Timeout();
}
