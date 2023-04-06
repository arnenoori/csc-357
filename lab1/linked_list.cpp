#include <iostream>
#include <cstring>

struct ListElement
{
    ListElement *next, *prev;
    char text[1000];
};

ListElement *head = NULL;

ListElement* getLastElement()
{
    ListElement *temp = head;
    while (temp && temp->next)
    {
        temp = temp->next;
    }
    return temp;
}

void pushString(const char *str)
{
    ListElement *newElement = new ListElement;
    strcpy(newElement->text, str);
    newElement->next = NULL;
    newElement->prev = getLastElement();
    if (newElement->prev)
    {
        newElement->prev->next = newElement;
    }
    else
    {
        head = newElement;
    }
}

void printList()
{
    ListElement *temp = head;
    while (temp)
    {
        std::cout << temp->text << std::endl;
        temp = temp->next;
    }
}

void deleteItem(int itemNumber)
{
    ListElement *temp = head;
    int count = 1;
    while (temp && count < itemNumber)
    {
        temp = temp->next;
        count++;
    }
    if (!temp)
    {
        std::cout << "Item not found" << std::endl;
        return;
    }
    if (temp->prev)
    {
        temp->prev->next = temp->next;
    }
    else
    {
        head = temp->next;
    }
    if (temp->next)
    {
        temp->next->prev = temp->prev;
    }
    delete temp;
}

void deleteList()
{
    ListElement *temp = head;
    while (temp)
    {
        ListElement *next = temp->next;
        delete temp;
        temp = next;
    }
    head = NULL;
}

int main()
{
    int choice;
    char inputString[1000];
    int itemNumber;

    while (true)
    {
        std::cout << "Select:" << std::endl;
        std::cout << "1 push string" << std::endl;
        std::cout << "2 print list" << std::endl;
        std::cout << "3 delete item" << std::endl;
        std::cout << "4 end program" << std::endl;
        std::cin >> choice;
        std::cin.ignore();
        switch (choice)
        {
        case 1:
            std::cout << "Insert text: ";
            std::cin.getline(inputString, sizeof(inputString));
            pushString(inputString);
            break;
        case 2:
            printList();
            break;
        case 3:
            std::cout << "Enter the item number to delete: ";
            std::cin >> itemNumber;
            std::cin.ignore();
            deleteItem(itemNumber);
            break;
        case 4:
            deleteList();
            return 0;
        default:
            std::cout << "Invalid choice. Please try again." << std::endl;
        }
    }

    return 0;
}
