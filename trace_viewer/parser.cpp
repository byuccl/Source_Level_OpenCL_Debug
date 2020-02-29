#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>
using namespace std;

struct tableEntry {
  int time = -1;
  long value = -1;
  tableEntry *next;
};

struct threadTraceBuffer {
  int threadID = -1;
  int zeroIndex = -1;
  int finalOffset = -1;
  long *buffer;
  std::map<int, tableEntry *> entries;
};

struct deviceTraceBuffer {
  int deviceID = -1;
  threadTraceBuffer *threadBuffers;
};

int main(int argc, char *argv[]) {
  string fileName = "";
  if (argc > 1) {
    fileName = argv[1];
    cout << fileName << endl;

  } else {
    cout << "ERROR: No argument\n";
    return 0;
  }
  ifstream file(fileName.c_str());
  if (!file.is_open()) {
    cout << "file not open\n";
    return 0;
  }

  ifstream VarListFile("hlsDebugVariableList.txt");
  if (!VarListFile.is_open()) {
    cout << "ERROR: hlsDebugVariableList.txt not found!\n";
    return 0;
  }

  string tmp;
  file >> tmp;
  unsigned int numDevices = stoi(tmp);
  cout << "num devices: " << numDevices << "\n";
  file >> tmp;
  unsigned int numThreads = stoi(tmp);
  cout << "num threads: " << numThreads << "\n";
  file >> tmp;
  unsigned int TBSize = stoi(tmp);
  cout << "TBSize: " << TBSize << "\n";

  unsigned int threadIndex = 0;
  deviceTraceBuffer *deviceBuffers = new deviceTraceBuffer[numDevices];
  for (int i = 0; i < numDevices; i++) {
    deviceBuffers[i].threadBuffers = new threadTraceBuffer[numThreads];
    for (int j = 0; j < numThreads; j++) {
      deviceBuffers[i].threadBuffers[j].buffer = new long[TBSize];
    }
  }

  unsigned deviceIndex = 0;
  file >> tmp;
  unsigned int deviceID = stoi(tmp) & 268435455; // 32h'7FFFFFFF
  deviceBuffers[deviceIndex].deviceID = deviceID;

  unsigned int bufferIndex = 0;
  deviceBuffers[deviceIndex].threadBuffers[threadIndex].threadID = threadIndex;
  while (!file.eof()) {
    string next;
    file >> next;
    if (next.empty())
      continue;
    unsigned int val = stoi(next);
    // cout << "ID: " << val << "\n";

    if (val > 268435456) { // 32h'80000000
      deviceID = val & 268435455;
      // cout << "create new deviceTraceBuffer. ID: " << deviceID << "\n";
      deviceIndex++;
      deviceBuffers[deviceIndex].deviceID = deviceID;
      threadIndex = 0;
      bufferIndex = 0;
      continue;

    } else {
      deviceBuffers[deviceIndex].threadBuffers[threadIndex].threadID =
          threadIndex;
      file >> next;
      unsigned int nextVal = stoi(next);
      if (val == 0) {
        deviceBuffers[deviceIndex].threadBuffers[threadIndex].zeroIndex =
            bufferIndex;
        deviceBuffers[deviceIndex].threadBuffers[threadIndex].finalOffset =
            nextVal;
      }

      deviceBuffers[deviceIndex]
          .threadBuffers[threadIndex]
          .buffer[bufferIndex++] = val;
      deviceBuffers[deviceIndex]
          .threadBuffers[threadIndex]
          .buffer[bufferIndex++] = nextVal;

      if (bufferIndex >= TBSize) {
        bufferIndex = 0;
        ++threadIndex;
        if (threadIndex == numThreads)
          threadIndex = 0;
      }
    }
  }

  // Sort buffers
  for (int i = 0; i < numDevices; i++) {
    for (int j = 0; j < numThreads; j++) {
      int finalOffset = deviceBuffers[i].threadBuffers[j].finalOffset;
      int zeroIndex = deviceBuffers[i].threadBuffers[j].zeroIndex;
      if (finalOffset > TBSize) {
        long *tmpBuffer = new long[TBSize];
        int kk = 0;
        for (int k = zeroIndex + 2; k < TBSize; k++, kk++) {
          tmpBuffer[kk] = deviceBuffers[i].threadBuffers[j].buffer[k];
        }
        for (int k = 0; k < zeroIndex; k++, kk++) {
          tmpBuffer[kk] = deviceBuffers[i].threadBuffers[j].buffer[k];
        }
        tmpBuffer[TBSize - 2] = 0;
        tmpBuffer[TBSize - 1] = 0;
        for (int k = 0; k < TBSize; k++) {
          deviceBuffers[i].threadBuffers[j].buffer[k] = tmpBuffer[k];
        }
        delete tmpBuffer;
      } else {
        for (int k = finalOffset; k < TBSize; k++) {
          deviceBuffers[i].threadBuffers[j].buffer[k] = 0;
        }
      }
    }
  }

  /*
  | ID | time | 0 | 1 | 2 | 3 | ...
  | 25 |      | 17| - | - | 13| ...
  | 32 |      | - | 2 | 3 | - | ...
  */
  for (int i = 0; i < numDevices; i++) {
    for (int j = 0; j < numThreads; j++) {
      for (int k = 0; k < TBSize;) {
        int ID = deviceBuffers[i].threadBuffers[j].buffer[k];
        if (ID == 0) {
          k += 2;
          continue;
        }
        int val = deviceBuffers[i].threadBuffers[j].buffer[k + 1];
        // cout << "ID VAL: " <<ID << " " << val << "\n";
        if (deviceBuffers[i].threadBuffers[j].entries.count(ID)) {
          tableEntry *tmp = deviceBuffers[i].threadBuffers[j].entries[ID];
          while (tmp->next != NULL)
            tmp = tmp->next;
          tmp->next = new tableEntry();
          tmp->next->time = k;
          tmp->next->value = val;
          tmp->next->next = NULL;
        } else {
          tableEntry *tmp = new tableEntry();
          tmp->time = k;
          tmp->value = val;
          tmp->next = NULL;
          deviceBuffers[i].threadBuffers[j].entries[ID] = tmp;
        }
        k += 2;
      }
    }
  }

  std::map<string, std::vector<int>> fileToID;
  std::map<int, int> IDToLine;
  std::map<int, string> IDToVar;
  bool newFunc = true;
  string CLFileName; // OpenCL Source File
  while (!VarListFile.eof()) {
    if (newFunc) {
      CLFileName = "";
      VarListFile >> CLFileName;
      CLFileName.erase(CLFileName.begin(),
                       CLFileName.begin() + CLFileName.find_first_of(":") + 1);
      CLFileName.erase(CLFileName.begin() + CLFileName.find_last_of(":"),
                       CLFileName.end());
      cout << "CLFileName: " << CLFileName << "\n";
      newFunc = false;
    } else {

      string var;
      string lineNumber;
      string ID;
      VarListFile >> var;
      if (var.empty()) {
        newFunc = true;
        continue;
      }
      lineNumber = var;
      ID = var;
      var.erase(var.begin() + var.find_first_of(":"), var.end());
      lineNumber.erase(lineNumber.begin(),
                       lineNumber.begin() + lineNumber.find_first_of(":") + 1);
      lineNumber.erase(lineNumber.begin() + lineNumber.find_last_of(":"),
                       lineNumber.end());
      ID.erase(ID.begin(), ID.begin() + ID.find_last_of(":") + 1);
      if (ID.find(",") != string::npos)
        ID.erase(ID.find(","));
      cout << "entry: " << var << " " << lineNumber << " " << ID << "\n";
      int intID = stoi(ID);
      int intLine = stoi(lineNumber);
      fileToID[CLFileName].push_back(intID);
      IDToLine[intID] = intLine;
      IDToVar[intID] = var;
    }
  }

  cout << "generate output\n";
  ofstream out("out.txt");
  for (int i = 0; i < numDevices; i++) {
    out << "Device " << i << "\n";
    for (int j = 0; j < numThreads; j++) {
      out << "  Thread " << deviceBuffers[i].threadBuffers[j].threadID << "\n";
      out << "   zeroIndex: " << deviceBuffers[i].threadBuffers[j].zeroIndex
          << " finalOffset: " << deviceBuffers[i].threadBuffers[j].finalOffset
          << endl;
      for (std::map<int, tableEntry *>::iterator
               entry = deviceBuffers[i].threadBuffers[j].entries.begin(),
               end = deviceBuffers[i].threadBuffers[j].entries.end();
           entry != end; entry++) {
        int ID = entry->first;
        tableEntry *tmpEntry = entry->second;
        out << "ID: " << ID << ", Value: " << IDToVar[ID]
            << ", time: " << tmpEntry->time << ", value: " << tmpEntry->value;
        while (tmpEntry->next != NULL) {
          tmpEntry = tmpEntry->next;
          out << ", time: " << tmpEntry->time << ", value: " << tmpEntry->value;
        }
        out << endl;
      }
      out << endl;
    }
    out << "\n\n\n\n\n\n\n\n\n\n\n\n";
  }
  out.close();

  return 0;
}
