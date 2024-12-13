#include <iostream>

int main() {
    std::cout << "Hello, World!" << std::endl;
    return 0;
}
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <regex>

#define docs "doctors.txt"
#define apps "appointments.txt"
#define docPrim "doctorsPrimaryIdx.txt"
#define docSec "doctorsSecondaryIdx.txt"
#define appPrim "appointmentsPrimaryIdx.txt"
#define appSec "appointmentsSecondaryIdx.txt"
#define docav "avalListDoc.txt"
#define appav "avalListApp.txt"



using namespace std;

class Doctor {
public:
    string doctor_id;
    string doctor_name;
    string doctor_address;
    Doctor(){}

    Doctor(string id, string name, string addr) {
        doctor_id = id;
        doctor_name = name;
        doctor_address = addr;
    }

    void print() {
        cout << "Doctor ID: " << doctor_id << endl
             << "Doctor Name: " << doctor_name << endl
             << "Doctor Address: " << doctor_address << endl;
    }

    void write(ofstream& outFile) {
        outFile << doctor_id << "|" << doctor_name << "|" << doctor_address << endl;
    }

    static Doctor read(ifstream& inFile) {
        string id, name, addr;
        if (getline(inFile, id, '|') && getline(inFile, name, '|') && getline(inFile, addr)) {
            return Doctor(id, name, addr);
        }
        return Doctor("", "", "");
    }
};

class Appointment {
public:
    string appointment_id;
    string appointment_date;
    string doctor_id;

    Appointment(){}
    Appointment(string appID, string date, string docID) {
        appointment_id = appID;
        appointment_date = date;
        doctor_id = docID;
    }

    void print() {
        cout << "Appointment id: " << appointment_id << endl
             << "Appointment date: " << appointment_date << endl
             << "Doctor id: " << doctor_id << endl;
    }

    void write(ofstream& outFile) {
        outFile << appointment_id << "|" << appointment_date << "|" << doctor_id << endl;
    }

    static Appointment read(ifstream& inFile) {
        string appID, date, docID;
        if (getline(inFile, appID, '|') && getline(inFile, date, '|') && getline(inFile, docID)) {
            return Appointment(appID, date, docID);
        }
        return Appointment("", "", "");
    }
};


struct Node {
    string id;
    Node* next;

    Node(string id) : id(id), next(nullptr) {}
};


vector<pair<string ,streampos>> doctorPrimaryIndex;
vector<pair<string ,streampos>> appointmentPrimaryIndex;
map<string, Node*> doctorSecondaryIndex;
map<string, Node*> appointmentSecondaryIndex;
//////////////////////////////////////////////////////////////////////////////

//avail list using best fit strategy
struct AvailNode {
    streampos offset;
    size_t size;
    AvailNode* next;
};

AvailNode* doctorAvailListHead = nullptr;
AvailNode* appointmentAvailListHead = nullptr;

void insertAvailNode(AvailNode*& head, streampos offset, size_t size) {
    AvailNode* newNode = new AvailNode{offset, size, nullptr};
    if (!head || head->size > size) { // Insert at the head
        newNode->next = head;
        head = newNode;
        return;
    }

    AvailNode* current = head;
    while (current->next && current->next->size < size) {
        current = current->next;
    }
    newNode->next = current->next;
    current->next = newNode;
}

AvailNode* findAndRemoveBestFit(AvailNode*& head, size_t recordLength) {
    AvailNode** current = &head;
    AvailNode* bestFit = *current;
    AvailNode** bestFitPrev = current;

    while (*current) {
        if ((*current)->size >= recordLength) {
            if ( (*current)->size < bestFit->size) {
                bestFit = *current;
                bestFitPrev = current;
                break;
            }
        }
        current = &((*current)->next);
    }

    if (bestFit) {
        *bestFitPrev = bestFit->next;
    }
    return bestFit;
}

void writeAvailListToFile(const string& filename) {
    fstream file(filename, ios::out );
    if (!file.is_open()) {
        cerr << "Error: Could not open the file to write avail list.\n";
        return;
    }

    AvailNode* current = doctorAvailListHead;
    while (current) {
