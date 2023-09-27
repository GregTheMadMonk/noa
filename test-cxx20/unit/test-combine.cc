// Standard library
#include <concepts>
#include <string>
#include <queue>

// Ensure compile-time testing of `common/meta` and `combine`
#define NOA_COMPILE_TIME_TESTS

// NOA headers
#include <noa-cxx20/combine/combine.hh>

// gtest
#include <gtest/gtest.h>

namespace combine = noa::utils::combine;

using combine::TaskCopy;
using combine::TaskMove;

// Event messages
class Msg {
    std::queue<std::string> msg{};
public:
    std::size_t size() const { return this->msg.size(); }

    void push(std::string_view sv) {
        msg.emplace(sv.data());
    }

    std::string pop() {
        if (msg.size() == 0) throw std::runtime_error{"No messages!"};
        const auto front = msg.front();
        msg.pop();
        return front;
    }
} msg;

// Tasks
struct Task1 {
    // Everything is public for the sake of testing
    int* storage = nullptr;
    int size = 2;
    bool isUpdated = false;

    static constexpr auto name = "Task1";

    Task1() {
        this->storage = new int[this->size];
        this->storage[0] = 10;
        this->storage[1] = 142;
        msg.push("Construct Task1");
    }
    Task1(TaskCopy, const Task1& other) {
        this->size = other.size;
        this->storage = new int[this->size];
        for (std::size_t i = 0; i < this->size; ++i) {
            this->storage[i] = other.storage[i];
        }
        this->isUpdated = other.isUpdated;

        msg.push("Copy Task1");
    }
    Task1(TaskMove, Task1&& other) {
        this->size = other.size;
        this->storage = other.storage;
        other.size = 0;
        other.storage = nullptr;

        this->isUpdated = other.isUpdated;

        msg.push("Move Task1");
    }

    Task1(const Task1&) = delete;
    Task1(Task1&&) = delete;
    Task1 operator=(const Task1&) = delete;
    Task1 operator=(Task1&&) = delete;

    ~Task1() {
        delete[] this->storage;
        msg.push("Destroy Task1");
    }

    void giveMore() {
        this->size = 3;
        int* newStorage = new int[this->size];
        newStorage[0] = this->storage[0];
        newStorage[1] = this->storage[1];
        delete[] this->storage;
        this->storage = newStorage;
        this->isUpdated = true;
    }

    int get(std::size_t idx) const { return this->storage[idx]; }
    void set(std::size_t idx, int value) {
        this->isUpdated = true;
        this->storage[idx] = value;
    }

    void run() {
        this->isUpdated = false;
        msg.push("Run Task1");
    }

    bool updated() const { return this->isUpdated; }
}; // <-- struct Task1

struct Task2 {
    static constexpr auto name = "Task2";
    int value = 0;
    int* storage = nullptr;

    Task2(const Task1& task1) {
        this->value = task1.get(1);
        this->storage = task1.storage + 1;
        msg.push(
            "Construct Task2 with value " + std::to_string(this->value)
        );
    }
    Task2(TaskCopy, const Task2& other, const Task1& task1) {
        this->value = other.value;
        this->storage = task1.storage + 1;
        msg.push("Copy Task2");
    }
    Task2(TaskMove, Task2&& other) {
        this->value = other.value;
        this->storage = other.storage;
        other.storage = nullptr;
        msg.push("Move Task2");
    }
    ~Task2() {
        msg.push("Destroy Task2");
    }

    void run(Task1& task1) {
        ++this->value;
        ++this->storage[0];
        ++task1.storage[0];
        msg.push("Run Task2");
    }

    void onUpdated(const std::same_as<Task1> auto& task1) {
        msg.push("Updated Task2 by Task1");
    }
}; // <-- struct Task2

constexpr auto initializer = [] (std::same_as<Task1> auto& t) {
    t.storage[0] = 42;
};

void testOneTask(auto& comp) {
    EXPECT_EQ(msg.pop(), "Construct Task1");

    auto& task1 = comp.template get<Task1>();
    EXPECT_EQ(task1.get(0), 42);
    EXPECT_EQ(task1.get(1), 142);

    EXPECT_FALSE(task1.updated());

    comp.run();

    EXPECT_EQ(msg.pop(), "Run Task1");

    task1.set(0, 256);
    EXPECT_TRUE(task1.updated());
    EXPECT_EQ(task1.get(0), 256);

    comp.run();
    EXPECT_EQ(msg.pop(), "Run Task1");
    EXPECT_FALSE(task1.updated());
    EXPECT_EQ(task1.get(0), 256);

    task1.giveMore();
    task1.set(2, 512);
    EXPECT_EQ(task1.get(0), 256);
    EXPECT_EQ(task1.get(1), 142);
    EXPECT_EQ(task1.get(2), 512);
    EXPECT_TRUE(task1.updated());

    comp.run();
    EXPECT_EQ(msg.pop(), "Run Task1");
    EXPECT_FALSE(task1.updated());

    auto compCopy = comp;
    auto& task1Copy = compCopy.template get<Task1>();
    EXPECT_EQ(msg.pop(), "Copy Task1");
    EXPECT_EQ(task1.size, 3);
    EXPECT_EQ(task1.size, task1Copy.size);
    task1.set(2, 1024);
    EXPECT_EQ(task1.get(0), 256);
    EXPECT_EQ(task1.get(1), 142);
    EXPECT_EQ(task1.get(2), 1024);
    EXPECT_EQ(task1Copy.get(0), 256);
    EXPECT_EQ(task1Copy.get(1), 142);
    EXPECT_EQ(task1Copy.get(2), 512);

    auto compMove = std::move(comp);
    auto& task1Move = compMove.template get<Task1>();
    EXPECT_EQ(msg.pop(), "Move Task1");
    EXPECT_EQ(task1.size, 0);
    EXPECT_EQ(task1.storage, nullptr);
    EXPECT_EQ(task1Move.size, 3);
    EXPECT_EQ(task1Move.get(0), 256);
    EXPECT_EQ(task1Move.get(1), 142);
    EXPECT_EQ(task1Move.get(2), 1024);
} // <-- void testOneTask(comp)

void testTwoTasks(auto& comp) {
    EXPECT_EQ(msg.pop(), "Construct Task1");
    EXPECT_EQ(msg.pop(), "Construct Task2 with value 142");

    comp.run();
    EXPECT_EQ(msg.pop(), "Run Task1");
    EXPECT_EQ(msg.pop(), "Run Task2");

    auto& task1 = comp.template get<Task1>();
    const auto& task2 = comp.template get<Task2>();
    EXPECT_EQ(task1.get(0), 43);
    EXPECT_EQ(task1.get(1), 143);
    EXPECT_EQ(task2.value, 143);

    task1.isUpdated = true;

    comp.run();
    EXPECT_EQ(msg.pop(), "Updated Task2 by Task1");
    EXPECT_EQ(msg.pop(), "Run Task1");
    EXPECT_EQ(msg.pop(), "Run Task2");

    auto compCopy = comp;
    EXPECT_EQ(msg.pop(), "Copy Task1");
    EXPECT_EQ(msg.pop(), "Copy Task2");

    compCopy.run();
    EXPECT_EQ(msg.pop(), "Run Task1");
    EXPECT_EQ(msg.pop(), "Run Task2");

    EXPECT_EQ(task1.get(0), 44);
    EXPECT_EQ(task1.get(1), 144);
    EXPECT_EQ(task2.value, 144);
    const auto& task1Copy = compCopy.template get<Task1>();
    const auto& task2Copy = compCopy.template get<Task2>();
    EXPECT_EQ(task1Copy.get(0), 45);
    EXPECT_EQ(task1Copy.get(1), 145);
    EXPECT_EQ(task2Copy.value, 145);

    auto compMove = std::move(comp);
    EXPECT_EQ(msg.pop(), "Move Task1");
    EXPECT_EQ(msg.pop(), "Move Task2");

    EXPECT_EQ(task1.storage, nullptr);
    EXPECT_EQ(task1.size, 0);
} // <-- void testTwoTasks(comp)

TEST(Combine, StaticComposer_OneTask) {
    {
        combine::StaticComposer<Task1> comp{ initializer };
        testOneTask(comp);
    }

    EXPECT_EQ(msg.pop(), "Destroy Task1");
    EXPECT_EQ(msg.pop(), "Destroy Task1");
    EXPECT_EQ(msg.pop(), "Destroy Task1");
} // <-- TEST(Combine, StaticComposer_OneTask)

TEST(Combine, StaticComposer_TwoTasks) {
    {
        combine::StaticComposer<Task2> comp{ initializer };
        testTwoTasks(comp);
    }

    EXPECT_EQ(msg.pop(), "Destroy Task1");
    EXPECT_EQ(msg.pop(), "Destroy Task2");
    EXPECT_EQ(msg.pop(), "Destroy Task1");
    EXPECT_EQ(msg.pop(), "Destroy Task2");
    EXPECT_EQ(msg.pop(), "Destroy Task1");
    EXPECT_EQ(msg.pop(), "Destroy Task2");
} // <-- TEST(Combine, StaticComposer_TwoTasks)

TEST(Combine, DynamicComposer_OneTask) {
    {
        combine::DynamicComposer<Task2> comp{};
        comp.setTasks({ "Task1" }, { initializer });

        testOneTask(comp);
    }

    EXPECT_EQ(msg.pop(), "Destroy Task1");
    EXPECT_EQ(msg.pop(), "Destroy Task1");
    EXPECT_EQ(msg.pop(), "Destroy Task1");
} // <-- TEST(Combine, StaticComposer_OneTask)

TEST(Combine, DynamicComposer_TwoTasks) {
    {
        combine::DynamicComposer<Task2> comp{};
        comp.setTasks({ "Task2" }, { initializer });

        testTwoTasks(comp);
    }

    EXPECT_EQ(msg.pop(), "Destroy Task1");
    EXPECT_EQ(msg.pop(), "Destroy Task2");
    EXPECT_EQ(msg.pop(), "Destroy Task1");
    EXPECT_EQ(msg.pop(), "Destroy Task2");
    EXPECT_EQ(msg.pop(), "Destroy Task1");
    EXPECT_EQ(msg.pop(), "Destroy Task2");
} // <-- TEST(Combine, StaticComposer_TwoTasks)
