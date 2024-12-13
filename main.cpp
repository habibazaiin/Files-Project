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
        file<<current->offset<<"|"<<current->size<<'\n';
        current = current->next;
    }

    file.close();
}

void loadAvailList(const string& filename,AvailNode* node) {
    fstream file(filename, ios::in | ios::out);
    if (!file.is_open()) {
        fstream file(filename, ios::out);
        cerr << "Could not open file, created new one.\n";
        return;
    }

    string line;
    while (getline(file, line)) {
        // Assuming the line contains offset and size, separated by a delimiter
        size_t delimiterPos = line.find('|');
        if (delimiterPos != string::npos) {
            AvailNode* newNode = new AvailNode;

            // Extract offset and size from the line
            newNode->offset = stoll(line.substr(0, delimiterPos));
            newNode->size = stoi(line.substr(delimiterPos + 1));

            newNode->next = nullptr;
            insertAvailNode(node, newNode->offset, newNode->size);
        }
    }

    file.close();
}
/////////////////////////////////////////////////////////////////
//Load indexes at start of program
void loadPrimaryIndexFromStorage(const string& indexFile, vector<pair<string, streampos>>& index) {

    fstream file(indexFile, ios::in | ios::out );
    if (!file.is_open()) {
        fstream file(indexFile, ios::out);

        cerr << "Can't open file,created new one. \n";

        return;
    }


    string line;
    while (getline(file, line)) {

        size_t delimiterPos = line.find('|');
        if (delimiterPos != string::npos) {
            string key = line.substr(0, delimiterPos);
            streampos offset = stoll(line.substr(delimiterPos + 1));


            index.push_back(make_pair(key, offset));
        }
    }

    file.close();
}

void loadSecondaryIndexFromStorage(const string& indexFile, map<string, Node*>& index) {
    fstream file(indexFile, ios::in | ios::out );
    if (!file.is_open()) {
        //create file if doesn't exist
        fstream file(indexFile, ios::out );

        cerr << "Can't open file,created new one. \n";
        return;
    }

    string line;
    while (getline(file, line)) {
        stringstream ss(line);
        string key;
        string valuesStr;

        getline(ss, key, '|');
        getline(ss, valuesStr);
        // Create the first node with the key
        Node* head = new Node(key);


        // Create the linked list for the values
        Node* current = head;
        stringstream valuesStream(valuesStr);
        string value;

        while (getline(valuesStream, value, '-')) {
            Node* nextNode = new Node(value);
            current->next = nextNode;
            current = nextNode;
        }
        index[key]=head->next;


    }

    file.close();
}

void loadAllIndexes() {
    loadPrimaryIndexFromStorage(docPrim, doctorPrimaryIndex);
    loadPrimaryIndexFromStorage(appPrim,appointmentPrimaryIndex);
    loadSecondaryIndexFromStorage(docSec,doctorSecondaryIndex);
    loadSecondaryIndexFromStorage(appSec,appointmentSecondaryIndex);
}


////////////////////////////////////////////////////////////////////////////////////////////////
//Primary index search
template <typename T>
typename vector<pair<string, streampos>>::iterator binarySearchPrimaryIdx(vector<pair<string, T>> &index,  string &key) {
    auto it = lower_bound(index.begin(), index.end(), make_pair(key, T(0)),
                          [](const pair<string, T> &a, const pair<string, T> &b) {
                              return a.first < b.first;
                          });

    if (it != index.end() && it->first == key) {
        return it;
    }
    return index.end();
}

//update and create indexes
void updatePrimaryIdx(  vector<pair<string, streampos>>& index) {

    sort(index.begin(), index.end(), [](const auto& a, const auto& b) {
        return a.first < b.first; // Ascending order by first element

    });

}

void updatePrimaryIdxFile(const string& indexFile,  vector<pair<string, streampos>>& index){
    ofstream file(indexFile, ios::trunc); //  overwrite file
    if (!file.is_open()) {
        cerr << "Could not open index file for writing.\n";
        return;
    }

    for (const auto& entry : index) {
        file << entry.first << "|" << entry.second << "\n";
    }

    file.close();
    cout << "Index file updated: " << indexFile << endl;
}

template <typename KeyType>

void updateSecondaryIdx( map<string, Node*>& index,
                         const KeyType& key, Node* node,bool isdeleted=false) {
    if (!isdeleted) {
        if (index.find(key) != index.end()) {
            Node *current = index[key];
            while (current->next) {
                current = current->next;
            }
            current->next = node;
        } else {
            index[key] = node;
        }
    } else {
        if (index.find(key) != index.end()) {
            Node *current = index[key];
            Node *prev = nullptr;


            if (current->id == node->id) {
                index[key] = current->next;
                delete current;
            } else {

                while (current && current->id != node->id) {
                    prev = current;
                    current = current->next;
                }

                // If node was found, unlink it from the list
                if (current) {
                    prev->next = current->next;  //
                    delete current;

                }

            }
        }
        if (!index[key]) {
            index.erase(key);
        }
    }
}

void updateSecondaryIdxFile(const string& indexFile, map<string, Node*>& index){
    ofstream file(indexFile, ios::trunc);
    if (!file.is_open()) {
        cerr << "Error: Could not open index file for writing.\n";
        return;
    }

    // Write each key and linked list to the file
    for (const auto& entry : index) {
        file << entry.first << "|";

        Node* current = entry.second;
        while (current) {
            file << current->id;
            if (current->next) file << "-";
            current = current->next;
        }

        file << "\n";
    }

    file.close();
    cout << "Index file updated: " << indexFile << endl;
}

///////////////////////////////////////////////////////////////////////////////////////////////
//add new record
void add_doctor() {
    Doctor doc;
    cout<<"Please enter the following doctor info: \n";
    cout<<"ID: \n";
    cin>>doc.doctor_id;
    if (binarySearchPrimaryIdx(doctorPrimaryIndex, doc.doctor_id) != doctorPrimaryIndex.end()) {
        cout << "Doctor ID already exists.\n";
        return;
    }
    cout<<"Name: \n";
    cin.ignore();
    getline(cin, doc.doctor_name);
    cout<<"Address: \n";
    getline(cin, doc.doctor_address);

    stringstream recordStream;

// Include length indicators for each field
    recordStream << doc.doctor_id.size() << "|" << doc.doctor_id << "|"
                 << doc.doctor_name.size() << "|" << doc.doctor_name << "|"
                 << doc.doctor_address.size() << "|" << doc.doctor_address;

    string recordBody = recordStream.str();

// Add a total length indicator at the beginning
    size_t recordLength = recordBody.size() + to_string(recordBody.size()).size() + 1;
    string finalRecord = to_string(recordLength) + "|" + recordBody;

    fstream file(docs, ios::app);
    if (!file.is_open()) {
        cout << "Error opening file.\n";
        return;
    }

    streampos offset=-1;

    //check if avail list have records
    if (doctorAvailListHead) {
        AvailNode* bestFit = findAndRemoveBestFit(doctorAvailListHead, recordLength);
        if (bestFit) {
            offset = bestFit->offset;
            writeAvailListToFile(docav);
        } else {
            //append to the end of file
            file.seekp(0, ios::end);
            offset = file.tellp();
        }
    } else {
        file.seekp(0, ios::end);
        offset = file.tellp();
    }




// Write the Record to File at the determined offset
    file.seekp(offset);
    file << finalRecord << endl;
    file.close();



    doctorPrimaryIndex.push_back(make_pair(doc.doctor_id, offset));

    updatePrimaryIdx(doctorPrimaryIndex);

    Node* newDoctor =new Node(doc.doctor_id);

    updateSecondaryIdx(doctorSecondaryIndex,doc.doctor_name,newDoctor);

    cout << "Doctor added successfully.\n";

}

void add_appointment() {
    Appointment app;
    cout << "Please enter the following appointment info: \n";
    cout << "ID: \n";
    cin >> app.appointment_id;

    // Check if the appointment ID already exists
    if (binarySearchPrimaryIdx(appointmentPrimaryIndex, app.appointment_id) != appointmentPrimaryIndex.end()) {
        cout << "Appointment ID already exists.\n";
        return;
    }

    cout << "Appointment Date (YYYY-MM-DD): \n";
    cin.ignore();
    getline(cin, app.appointment_date);

    cout << "Doctor ID: \n";
    getline(cin, app.doctor_id);

    // Check if the doctor ID exists
    if (binarySearchPrimaryIdx(doctorPrimaryIndex, app.doctor_id) == doctorPrimaryIndex.end()) {
        cout << "Doctor ID does not exist.\n";
        return;
    }

    stringstream recordStream;

    // Include length indicators for each field
    recordStream << app.appointment_id.size() << "|" << app.appointment_id << "|"
                 << app.appointment_date.size() << "|" << app.appointment_date << "|"
                 << app.doctor_id.size() << "|" << app.doctor_id;

    string recordBody = recordStream.str();

    // Add a total length indicator at the beginning
    size_t recordLength = recordBody.size() + to_string(recordBody.size()).size() + 1;
    string finalRecord = to_string(recordLength) + "|" + recordBody;

    fstream file(apps, ios::app);
    if (!file.is_open()) {
        cout << "Error opening file.\n";
        return;
    }

    streampos offset = -1;

    // Check if there are available slots
    if (appointmentAvailListHead) {
        AvailNode* bestFit = findAndRemoveBestFit(appointmentAvailListHead, recordLength);
        if (bestFit) {
            offset = bestFit->offset;
            writeAvailListToFile(appav);
        } else {
            file.seekp(0, ios::end);
            offset = file.tellp();
        }
    } else {
        file.seekp(0, ios::end);
        offset = file.tellp();
    }

    // Write the record to the file at the determined offset
    file.seekp(offset);
    file << finalRecord << endl;
    file.close();

    // Update primary index
    appointmentPrimaryIndex.push_back(make_pair(app.appointment_id, offset));
    updatePrimaryIdx(appointmentPrimaryIndex);

    // Update secondary index for doctor ID
    Node* appNode = new Node(app.appointment_id);
    updateSecondaryIdx(appointmentSecondaryIndex, app.doctor_id, appNode);

    cout << "Appointment added successfully.\n";
}

//delete record
void delete_doctor() {
    string doctor_id;
    cout<<"Enter doctor id to delete.\n";
    cin>>doctor_id;
    auto it = binarySearchPrimaryIdx(doctorPrimaryIndex, doctor_id);

    if (it == doctorPrimaryIndex.end()) {
        cout << "Doctor ID does not exist.\n";
        return;
    }

    // Locate the record in the file
    fstream file(docs, ios::in | ios::out);
    if (!file.is_open()) {
        cerr << "Error: Could not open doctors file.\n";
        return;
    }
    auto offset= it->second;

    file.seekp(offset);
    char marker;
    file.get(marker);

    if (marker == '*') {
        cout << "Doctor record already marked as deleted.\n";
        file.close();
        return;
    }
    file.seekg(offset);
    size_t recordSize;
    file >> recordSize;  // Read the size of the record
    file.ignore();

    string record;
    getline(file, record);
    stringstream ss(record);
    string field;
    vector<string> fields;
    while (getline(ss, field, '|')) {
        fields.push_back(field);
    }


    string doctor_name = fields[3];


    file.seekp(offset);
    file.put('*');
    file.close();


    insertAvailNode(doctorAvailListHead,offset,recordSize);
    writeAvailListToFile(docav);

    // Remove from primary index
    doctorPrimaryIndex.erase(it);
    updatePrimaryIdx(doctorPrimaryIndex);

    // Remove from secondary index
    Node* doc =new Node(doctor_id);

    updateSecondaryIdx(doctorSecondaryIndex,doctor_name,doc, true);


    cout << "Doctor record deleted successfully.\n";
}

void delete_appointment() {
    string appointment_id;
    cout << "Enter appointment ID to delete.\n";
    cin >> appointment_id;

    auto it = binarySearchPrimaryIdx(appointmentPrimaryIndex, appointment_id);

    if (it == appointmentPrimaryIndex.end()) {
        cout << "Appointment ID does not exist.\n";
        return;
    }

    // Locate the record in the file
    fstream file(apps, ios::in | ios::out);
    if (!file.is_open()) {
        cerr << "Error: Could not open appointments file.\n";
        return;
    }

    auto offset = it->second;

    file.seekp(offset);
    char marker;
    file.get(marker);

    if (marker == '*') {
        cout << "Appointment record already marked as deleted.\n";
        file.close();
        return;
    }

    file.seekg(offset);
    size_t recordSize;
    file >> recordSize;
    file.ignore();

    string record;
    getline(file, record);
    stringstream ss(record);
    string field;
    vector<string> fields;
    while (getline(ss, field, '|')) {
        fields.push_back(field);
    }

    string doctor_id = fields[5];

    file.seekp(offset);
    file.put('*');
    file.close();

    insertAvailNode(appointmentAvailListHead, offset, recordSize);
    writeAvailListToFile(appav);

    // Remove from primary index
    appointmentPrimaryIndex.erase(it);
    updatePrimaryIdx(appointmentPrimaryIndex);

    // Remove from secondary index
    Node* appNode = new Node(appointment_id);
    updateSecondaryIdx(appointmentSecondaryIndex, doctor_id, appNode, true);

    cout << "Appointment record deleted successfully.\n";
}

//update functions
void updateDoctorName() {
    string doctorID;
    cout << "Enter doctor ID: ";
    cin >> doctorID;
    string currentName;

    // Find the doctor's record in the primary index
    auto it = binarySearchPrimaryIdx(doctorPrimaryIndex, doctorID);
    if (it == doctorPrimaryIndex.end() || it->first != doctorID) {
        cout << "Doctor ID doesn't exist.\n";
        return;
    }

    streampos offset = it->second;
    fstream file(docs, ios::in | ios::out);
    if (!file.is_open()) {
        cerr << "Error: Could not open the doctors file.\n";
        return;
    }

    // Read the existing record
    file.seekg(offset);
    string record;
    getline(file, record);

    // Parse the record to extract fields
    stringstream recordStream(record);
    string fieldLength, doctorIDLength, doctor_ID, doctorNameLength, doctorName, doctorAddressLength, doctorAddress;

    getline(recordStream, fieldLength, '|');
    getline(recordStream, doctorIDLength, '|');
    getline(recordStream, doctor_ID, '|');
    getline(recordStream, doctorNameLength, '|');
    getline(recordStream, doctorName, '|');
    getline(recordStream, doctorAddressLength, '|');
    getline(recordStream, doctorAddress);

    currentName=doctorName;
    // Update the doctor name
    cout << "Enter new doctor name: ";
    cin.ignore();
    string newDoctorName;
    getline(cin, newDoctorName);

    if (newDoctorName.size() > stoi(doctorNameLength)) {
        cout << "Error: New doctor name exceeds the allocated size.\n";
        return;
    }

    // Pad the new name if shorter
    newDoctorName.resize(stoi(doctorNameLength), ' ');

    // Construct the updated record
    stringstream updatedRecordStream;
    updatedRecordStream << fieldLength << "|" << doctorIDLength << "|" << doctorID << "|"
                        << doctorNameLength << "|" << newDoctorName << "|"
                        << doctorAddressLength << "|" << doctorAddress;
    string updatedRecord = updatedRecordStream.str();


    Node* doc =new Node(doctorID);
    //delete current name then add new name

    updateSecondaryIdx(doctorSecondaryIndex,currentName,doc, true);
    updateSecondaryIdx(doctorSecondaryIndex,newDoctorName,doc, false);

    // Write the updated record back to the file
    file.seekp(offset);
    file << updatedRecord << '\n';
    file.close();

    cout << "Doctor's name updated successfully.\n";
}

void updateAppointmentDate() {
    string appointmentID;
    cout << "Enter appointment ID: ";
    cin >> appointmentID;
    string currentDate;

    // Find the appointment's record in the primary index
    auto it = binarySearchPrimaryIdx(appointmentPrimaryIndex, appointmentID);
    if (it == appointmentPrimaryIndex.end() || it->first != appointmentID) {
        cout << "Appointment ID doesn't exist.\n";
        return;
    }

    streampos offset = it->second;
    fstream file(apps, ios::in | ios::out);
    if (!file.is_open()) {
        cerr << "Error: Could not open the appointments file.\n";
        return;
    }

    // Read the existing record
    file.seekg(offset);
    string record;
    getline(file, record);

    // Parse the record to extract fields
    stringstream recordStream(record);
    string fieldLength, appointmentIDLength, appointment_ID, appointmentDateLength, appointmentDate, patientIDLength, patientID;

    getline(recordStream, fieldLength, '|');
    getline(recordStream, appointmentIDLength, '|');
    getline(recordStream, appointment_ID, '|');
    getline(recordStream, appointmentDateLength, '|');
    getline(recordStream, appointmentDate, '|');
    getline(recordStream, patientIDLength, '|');
    getline(recordStream, patientID);

    currentDate = appointmentDate;

    // Update the appointment date
    cout << "Enter new appointment date (YYYY-MM-DD): ";
    cin.ignore();
    string newAppointmentDate;
    getline(cin, newAppointmentDate);

    if (newAppointmentDate.size() > stoi(appointmentDateLength)) {
        cout << "Error: New appointment date exceeds the allocated size.\n";
        return;
    }

    // Pad the new date if shorter
    newAppointmentDate.resize(stoi(appointmentDateLength), ' ');

    // Construct the updated record
    stringstream updatedRecordStream;
    updatedRecordStream << fieldLength << "|" << appointmentIDLength << "|" << appointmentID << "|"
                        << appointmentDateLength << "|" << newAppointmentDate << "|"
                        << patientIDLength << "|" << patientID;
    string updatedRecord = updatedRecordStream.str();

    // Update secondary index with old and new appointment date
    Node* appNode = new Node(appointmentID);
    updateSecondaryIdx( appointmentSecondaryIndex, currentDate, appNode, true);  // Remove old date
    updateSecondaryIdx( appointmentSecondaryIndex, newAppointmentDate, appNode, false);  // Add new date

    // Write the updated record back to the file
    file.seekp(offset);
    file << updatedRecord << '\n';
    file.close();

    cout << "Appointment date updated successfully.\n";
}
//////////////////////////////////////////////////////////////////

//print info
void printDoctorInfo(string doctorID= "") {
    if(doctorID=="") {
        cout << "Enter doctor ID: ";
        cin >> doctorID;
    }
    // Find the doctor's record in the primary index
    auto it = binarySearchPrimaryIdx(doctorPrimaryIndex, doctorID);
    if (it == doctorPrimaryIndex.end() || it->first != doctorID) {
        cout << "Doctor ID doesn't exist.\n";
        return;
    }

    streampos offset = it->second;
    fstream file(docs, ios::in);
    if (!file.is_open()) {
        cerr << "Error: Could not open the doctors file.\n";
        return;
    }

    // Read the existing record
    file.seekg(offset);
    string record;
    getline(file, record);

    // Parse the record to extract fields
    stringstream recordStream(record);
    string fieldLength, doctorIDLength, doctor_ID, doctorNameLength, doctorName, doctorAddressLength, doctorAddress;

    getline(recordStream, fieldLength, '|');
    getline(recordStream, doctorIDLength, '|');
    getline(recordStream, doctor_ID, '|');
    getline(recordStream, doctorNameLength, '|');
    getline(recordStream, doctorName, '|');
    getline(recordStream, doctorAddressLength, '|');
    getline(recordStream, doctorAddress);

    cout << "Doctor ID: " << doctor_ID << '\n';
    cout << "Doctor Name: " << doctorName << '\n';
    cout << "Doctor Address: " << doctorAddress << '\n';

    file.close();
}

void printAppointmentInfo(string appointmentID="") {
    if( appointmentID=="") {
        cout << "Enter appointment ID: ";
        cin >> appointmentID;
    }
    // Find the appointment's record in the primary index
    auto it = binarySearchPrimaryIdx(appointmentPrimaryIndex, appointmentID);
    if (it == appointmentPrimaryIndex.end() || it->first != appointmentID) {
        cout << "Appointment ID doesn't exist.\n";
        return;
    }

    streampos offset = it->second;
    fstream file(apps, ios::in);
    if (!file.is_open()) {
        cerr << "Error: Could not open the appointments file.\n";
        return;
    }

    // Read the existing record
    file.seekg(offset);
    string record;
    getline(file, record);

    // Parse the record to extract fields
    stringstream recordStream(record);
    string fieldLength, appointmentIDLength, appointment_ID, appointmentDateLength, appointmentDate, doctorIDLength, doctorID;

    getline(recordStream, fieldLength, '|');
    getline(recordStream, appointmentIDLength, '|');
    getline(recordStream, appointment_ID, '|');
    getline(recordStream, appointmentDateLength, '|');
    getline(recordStream, appointmentDate, '|');
    getline(recordStream, doctorIDLength, '|');
    getline(recordStream, doctorID);

    // Print appointment information
    cout << "Appointment ID: " << appointment_ID << endl;
    cout << "Appointment Date: " << appointmentDate << endl;
    cout << "Doctor ID: " << doctorID << endl;

    file.close();
}

//search secondary index
void searchDoctorByName(const string& doctorName) {
    auto it = doctorSecondaryIndex.find(doctorName);

    if (it == doctorSecondaryIndex.end()) {
        cout << "No doctor with this name.\n";
        return;
    }

    cout << "Doctors found with this name:\n";

    // Traverse the linked list in the secondary index
    Node* currentNode = it->second;

    fstream file(docs, ios::in);
    if (!file.is_open()) {
        cerr << "Error: Could not open the doctors file.\n";
        return;
    }

    while (currentNode) {
        // Locate the doctor record using the primary index
        auto primaryIt = binarySearchPrimaryIdx(doctorPrimaryIndex, currentNode->id);
        if (primaryIt != doctorPrimaryIndex.end()) {
            streampos offset = primaryIt->second;
            file.seekg(offset);

            // Read the record
            string record;
            getline(file, record);

            // Parse the record using length indicators
            stringstream recordStream(record);
            string fieldLength, doctorIDLength, doctorID, doctorNameLength, doctorName, addressLength, address;

            getline(recordStream, fieldLength, '|');
            getline(recordStream, doctorIDLength, '|');
            getline(recordStream, doctorID, '|');
            getline(recordStream, doctorNameLength, '|');
            getline(recordStream, doctorName, '|');
            getline(recordStream, addressLength, '|');
            getline(recordStream, address);

            // Output the doctor details
            Doctor doc(doctorID, doctorName, address);
            doc.print();
        }

        // Move to the next node in the linked list
        currentNode = currentNode->next;
    }

    file.close();
}

void searchAppointmentByDoctorID( const string& doctorID) {
    auto it = appointmentSecondaryIndex.find(doctorID);

    if (it == appointmentSecondaryIndex.end()) {
        cout << "No appointments found for this doctor ID.\n";
        return;
    }

    cout << "Appointments for Doctor ID: " << doctorID << ":\n";

    // Traverse the linked list in the secondary index
    Node* currentNode = it->second;

    fstream file(apps, ios::in);
    if (!file.is_open()) {
        cerr << "Error: Could not open the appointments file.\n";
        return;
    }

    while (currentNode) {
        // Locate the appointment record using the primary index
        auto primaryIt = binarySearchPrimaryIdx(appointmentPrimaryIndex, currentNode->id);
        if (primaryIt != appointmentPrimaryIndex.end()) {
            streampos offset = primaryIt->second;
            file.seekg(offset);

            // Read the record
            string record;
            getline(file, record);

            // Parse the record using length indicators
            stringstream recordStream(record);
            string fieldLength, appointmentIDLength, appointmentID, appointmentDateLength, appointmentDate, doctorIDLength, doctorID;

            getline(recordStream, fieldLength, '|');
            getline(recordStream, appointmentIDLength, '|');
            getline(recordStream, appointmentID, '|');
            getline(recordStream, appointmentDateLength, '|');
            getline(recordStream, appointmentDate, '|');
            getline(recordStream, doctorIDLength, '|');
            getline(recordStream, doctorID);

            // Output the appointment details
            Appointment app(appointmentID,appointmentDate,doctorID);
            app.print();

        }

        // Move to the next node in the linked list
        currentNode = currentNode->next;
    }

    file.close();
}


//write qeury
void execute_query() {
    string query;
    cout<<"please enter query in this forumla: Select column_name from table where cond\n";
    cin.ignore();
    getline(cin,query);
    stringstream ss(query);
    string word;

    // Parse the query into components
    string command, table, column_name, cond, value;
    ss >> command; // Select
    ss >> column_name; // e.g., "all"
    ss >> word; // from
    ss >> table; // Table name, e.g., "Doctors"
    ss >> word; // where
    ss >> cond; //eg Doctor ID
    ss>> word; // =
    ss >> value; // The value of Doctor ID, e.g., 'xxx'


    // Handle the query based on the table and column requested
    if (command !=  "Select" ) {
        cout<< "Invalid query format, only select is provided\n";
        return;
    }
    if (column_name == "all") {
        if (table=="Doctors"&&cond == "DoctorID") {
            auto it = binarySearchPrimaryIdx(doctorPrimaryIndex, value);
            printDoctorInfo(it->first);
        }
        else if (table=="Appointments"&& cond=="AppointmentID" ){
            auto it = binarySearchPrimaryIdx(appointmentPrimaryIndex, value);
            printAppointmentInfo(it->first);
        }
        else if(table=="Appointments"&& cond=="DoctorID"){
            searchAppointmentByDoctorID(value);
        }
    }
    else if(column_name=="DoctorName") {
        searchDoctorByName(value);
    }

}

void showMenu() {
    cout << "Welcome to Healthcare Management System\n";
    cout << "1. Add New Doctor\n";
    cout << "2. Add New Appointment\n";
    cout << "3. Update Doctor Name (by Doctor ID)\n";
    cout << "4. Update Appointment Date (by Appointment ID)\n";
    cout << "5. Delete Appointment (by Appointment ID)\n";
    cout << "6. Delete Doctor (by Doctor ID)\n";
    cout << "7. Print Doctor Info (by Doctor ID)\n";
    cout << "8. Print Appointment Info (by Appointment ID)\n";
    cout << "9. Write Query\n";
    cout << "10. Exit\n";
}
int main()
{
    loadAllIndexes();
    loadAvailList(docav,doctorAvailListHead);
    loadAvailList(appav,appointmentAvailListHead);

    int choice;
    do {
        showMenu();
        cout << "Enter your choice: ";
        cin >> choice;
        switch (choice) {
            case 1:
                add_doctor();
                break;
            case 2:
                add_appointment();
                break;
            case 3:
                updateDoctorName();
                break;
            case 4:
                updateAppointmentDate();
                break;
            case 5:
                delete_appointment();
                break;
            case 6:
                delete_doctor();
                break;
            case 7:
                printDoctorInfo();
                break;
            case 8:
                printAppointmentInfo();
                break;
            case 9:
                execute_query();
                break;
            case 10: cout << "Exiting...\n"; break;
            default: cout << "Invalid choice. Please try again.\n";
        }

    } while (choice != 10);
    updatePrimaryIdxFile(docPrim,doctorPrimaryIndex);
    updateSecondaryIdxFile(docSec,doctorSecondaryIndex);
    updatePrimaryIdxFile(appPrim,appointmentPrimaryIndex);
    updateSecondaryIdxFile(appSec,appointmentSecondaryIndex);
    return 0;



}
