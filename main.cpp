#include <iostream>
#include <fstream>
#include <unordered_map>
#include <string>
#include <chrono>
#include <memory>

size_t totalAllocatedMemory = 0;
size_t currentMemoryUsage = 0;

void* operator new(size_t size) {
    totalAllocatedMemory += size;
    currentMemoryUsage += size;
    return malloc(size);
}

void operator delete(void* memory, size_t size) {
    currentMemoryUsage -= size;
    free(memory);
}

constexpr const char* path = "serialized_list.txt";
constexpr const char* test_path = "test_serialized_list.txt";

struct ListNode
{
    ListNode* Prev;
    ListNode* Next;
    ListNode* Rand;
    std::string Data;

    ListNode(const std::string& data) : Prev(nullptr), Next(nullptr), Rand(nullptr), Data(data) {}
};

class ListRand
{
public:
    ListNode* Head;
    ListNode* Tail;
    int Count;

    int GetNodeIndex(ListNode* node) const
    {
        int index = 0;
        ListNode* current = Head;
        while (current != nullptr)
        {
            if (current == node)
                return index;
            current = current->Next;
            index++;
        }
        return -1; // Node not found
    }

    ListNode* GetNodeAtIndex(int index) const
    {
        if (index < 0 || index >= Count)
            return nullptr;

        ListNode* current = Head;
        for (int i = 0; i < index; i++)
        {
            if (current == nullptr)
                return nullptr;
            current = current->Next;
        }
        return current;
    }

    void Serialize(std::ofstream& s)
    {
        // Use a dictionary to map nodes to their indices during serialization
        std::unordered_map<ListNode*, int> nodeToIndex;
        int index = 0;
        ListNode* current = Head;

        // First pass: serialize the node data and build the dictionary
        while (current != nullptr)
        {
            nodeToIndex[current] = index;
            s << current->Data << '\n';
            current = current->Next;
            index++;
        }

        s << "-1\n"; // Mark the end of the data section

        // Second pass: serialize the Rand indices
        current = Head;
        while (current != nullptr)
        {
            if (current->Rand != nullptr)
            {
                int randIndex = nodeToIndex[current->Rand];
                s << randIndex << '\n';
            }
            else
            {
                s << "-1\n"; // No Rand reference for this node
            }
            current = current->Next;
        }
    }

    void Deserialize(std::ifstream& s)
    {
        // Clear the existing list if any
        while (Head != nullptr)
        {
            ListNode* next = Head->Next;
            delete Head;
            Head = next;
        }

        // Use a dictionary to map indices to nodes during deserialization
        std::unordered_map<int, ListNode*> indexToNode;

        std::string line;
        int index = 0;

        // First pass: deserialize the node data and build the dictionary
        while (std::getline(s, line))
        {
            if (line == "-1")
                break;

            ListNode* newNode = new ListNode(line);
            indexToNode[index] = newNode;

            if (index == 0)
            {
                Head = newNode;
                Tail = newNode;
            }
            else
            {
                Tail->Next = newNode;
                newNode->Prev = Tail;
                Tail = newNode;
            }
            index++;
        }

        // Second pass: deserialize the Rand indices and establish Rand references
        ListNode* current = Head;
        index = 0;
        while (std::getline(s, line))
        {
            if (line == "-1")
            {
                if (current)
                    current = current->Next;
                index++;
                continue;
            }

            int randIndex = std::stoi(line);
            if (randIndex >= 0 && randIndex < index)
            {
                ListNode* randNode = indexToNode[randIndex];
                if (current)
                    current->Rand = randNode;
            }

            if (current)
                current = current->Next;
            index++;
        }

        Count = index;
    }

    ListRand() : Head(nullptr), Tail(nullptr), Count(0) {}

    ~ListRand()
    {
        while (Head != nullptr)
        {
            ListNode* next = Head->Next;
            delete Head;
            Head = next;
        }
    }
};

ListRand GenerateRandomList(int size)
{
    ListRand list;

    ListNode* prevNode = nullptr;
    for (size_t i = 0; i < size; i++)
    {
        std::string data = "Node " + std::to_string(i + 1);
        ListNode* newNode = new ListNode(data);

        if (i == 0)
            list.Head = newNode;
        else
        {
            prevNode->Next = newNode;
            newNode->Prev = prevNode;
        }

        prevNode = newNode;
    }

    // Add random Rand references
    ListNode* current = list.Head;
    while (current != nullptr)
    {
        int randIndex = rand() % size;
        current->Rand = list.GetNodeAtIndex(randIndex);
        current = current->Next;
    }

    list.Tail = prevNode;
    list.Count = size;

    return list;
}

void TestSerializationDeserialization()
{
    std::ofstream outFile(test_path);
    if (!outFile.is_open())
    {
        std::cout << "Error: Failed to open output file for testing." << std::endl;
        return;
    }

    const int testSizes[] = { 100, 1000, 10000, 100000 };
    for (size_t size : testSizes)
    {
        ListRand testList = GenerateRandomList(size);

        auto startSerialize = std::chrono::high_resolution_clock::now();
        testList.Serialize(outFile);
        auto endSerialize = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsedSerialize = endSerialize - startSerialize;
        std::cout << "Serialization Time for " << size << " elements: " << elapsedSerialize.count() << " seconds" << std::endl;

        outFile.close();

        std::ifstream inFile(test_path);
        if (!inFile.is_open())
        {
            std::cout << "Error: Failed to open input file for testing." << std::endl;
            return;
        }

        ListRand deserializedList;
        auto startDeserialize = std::chrono::high_resolution_clock::now();
        deserializedList.Deserialize(inFile);
        auto endDeserialize = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsedDeserialize = endDeserialize - startDeserialize;
        std::cout << "Deserialization Time for " << size << " elements: " << elapsedDeserialize.count() << " seconds" << std::endl;

        inFile.close();

        while (testList.Head != nullptr)
        {
            ListNode* next = testList.Head->Next;
            delete testList.Head;
            testList.Head = next;
        }

        while (deserializedList.Head != nullptr)
        {
            ListNode* next = deserializedList.Head->Next;
            delete deserializedList.Head;
            deserializedList.Head = next;
        }
    }
}


int main()
{
    {
        ListRand list;
        ListNode* node1 = new ListNode("Node 1");
        ListNode* node2 = new ListNode("Node 2");
        ListNode* node3 = new ListNode("Node 3");

        node1->Next = node2;
        node2->Prev = node1;
        node2->Next = node3;
        node3->Prev = node2;

        node1->Rand = node3;
        node2->Rand = node1;
        node3->Rand = node2;

        list.Head = node1;
        list.Tail = node3;
        list.Count = 3;

        std::ofstream outFile(path);
        if (outFile.is_open())
        {
            list.Serialize(outFile);
            outFile.close();
        }

        std::ifstream inFile(path);
        if (inFile.is_open())
        {
            ListRand deserializedList;
            deserializedList.Deserialize(inFile);
            inFile.close();
        }
    }

    // Performance test
    srand(static_cast<unsigned int>(time(0)));
    TestSerializationDeserialization();

    std::cout << "Total allocated memory: " << totalAllocatedMemory << " bytes" << std::endl;
    std::cout << "Current memory usage: " << currentMemoryUsage << " bytes" << std::endl;

    return 0;
}
