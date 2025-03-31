#include <iostream>
#include <vector>
#include <algorithm>
#include <time.h>
#include <sstream>
#include <map>
#include <set>
#include <fstream>
#include "../include/course.h"  // Include header file for courses
#include "../include/room.h"    // Include header file for rooms

using namespace std;

struct Timeslot
{
    int day;    // Day of the week (1-5)
    int slot;   // Slot within the day (1-5)

    // Custom comparator for sorting Timeslots
    friend bool operator<(const Timeslot &lhs, const Timeslot &rhs)
    {
        if (lhs.day == rhs.day)
        {
            return lhs.slot < rhs.slot;
        }
        return lhs.day < rhs.day;
    }
};

struct Lecture
{
    string courseCode;  // Code of the course
    string faculty;     // Name of the faculty teaching the course
    string roomNumber;  // Room number where the lecture is scheduled
    Timeslot timeslot;  // Timeslot for the lecture
    string batch;       // Batch of students attending the lecture
    string branch;      // Branch(es) associated with the course
};

// Function to check if two timeslots overlap
bool isOverlap(const Timeslot &t1, const Timeslot &t2)
{
    return (t1.day == t2.day && t1.slot == t2.slot);
}

// Function to check if a lecture assignment is valid
bool isValidAssignment(const Lecture &lecture, const vector<Lecture> &timetable, map<Timeslot, set<string>> &timeslotOccupancy)
{
    auto splitBranches = [](const string &branches)
    {
        set<string> result;
        stringstream ss(branches);
        string branch;
        while (getline(ss, branch, ','))
        {
            result.insert(branch);
        }
        return result;
    };

    // Iterate over existing lectures in the timetable
    for (const auto &existingLecture : timetable)
    {
        set<string> commonBranches;
        set<string> lectureBranchSet = splitBranches(lecture.branch);
        set<string> existingLectureBranchSet = splitBranches(existingLecture.branch);

        // Calculate the intersection of branch sets
        set_intersection(
            lectureBranchSet.begin(), lectureBranchSet.end(),
            existingLectureBranchSet.begin(), existingLectureBranchSet.end(),
            inserter(commonBranches, commonBranches.begin()));

        // Check for conflicts in schedule
        if (isOverlap(lecture.timeslot, existingLecture.timeslot) &&
            (lecture.roomNumber == existingLecture.roomNumber ||
             lecture.faculty == existingLecture.faculty ||
             lecture.courseCode == existingLecture.courseCode ||
             (lecture.batch == existingLecture.batch && !commonBranches.empty())))
        {
            return false;  // Conflict found
        }
    }

    // Check if the timeslot is already occupied by another branch
    if (timeslotOccupancy[lecture.timeslot].find(lecture.branch) != timeslotOccupancy[lecture.timeslot].end())
    {
        return false;  // Timeslot already occupied
    }

    // Check for batch-specific conflicts
    for (const auto &existingLecture : timetable)
    {
        set<string> commonBranches;
        set<string> lectureBranchSet = splitBranches(lecture.branch);
        set<string> existingLectureBranchSet = splitBranches(existingLecture.branch);

        set_intersection(
            lectureBranchSet.begin(), lectureBranchSet.end(),
            existingLectureBranchSet.begin(), existingLectureBranchSet.end(),
            inserter(commonBranches, commonBranches.begin()));

        if (isOverlap(lecture.timeslot, existingLecture.timeslot) &&
            lecture.batch == existingLecture.batch &&
            !commonBranches.empty())
        {
            return false;  // Conflict found
        }
    }

    return true;  // Assignment is valid
}

// Function to assign timeslots to courses
vector<Timeslot> assignTimeslots(const vector<Course> &courses)
{
    vector<Timeslot> timeslots;
    for (const auto &course : courses)
    {
        for (int slot = 1; slot <= 5; ++slot)
        {
            Timeslot timeslot;
            timeslot.day = rand() % 5 + 1;  // Randomly assign day (1-5)
            timeslot.slot = slot;           // Assign slot (1-5)
            timeslots.push_back(timeslot);
        }
    }
    return timeslots;
}

// Function to generate timetable
vector<Lecture> generateTimetable(const vector<Course> &courses, const vector<Room> &rooms)
{
    vector<Lecture> timetable;
    map<Timeslot, set<string>> timeslotOccupancy;
    map<int, set<string>> lecturesAssignedPerDay;

    vector<Timeslot> timeslots = assignTimeslots(courses);
    random_shuffle(timeslots.begin(), timeslots.end());

    map<string, int> courseAssignments;

    // Iterate over courses
    for (const auto &course : courses)
    {
        int requiredLectures = stoi(course.credit.substr(0, course.credit.find("-")));
        int assignedLectures = 0;

        // Iterate over branches of the course
        for (const auto &branch : course.branches)
        {
            // Try to assign required number of lectures
            for (int i = 0; i < requiredLectures; ++i)
            {
                // Iterate over available timeslots
                for (const auto &timeslot : timeslots)
                {
                    // Iterate over available rooms
                    for (const auto &room : rooms)
                    {
                        Lecture lecture = {course.code, course.faculty, room.roomNumber, timeslot, course.batch, branch};

                        // Check if assignment is valid
                        if (isValidAssignment(lecture, timetable, timeslotOccupancy) &&
                            find(course.branches.begin(), course.branches.end(), branch) != course.branches.end() &&
                            lecturesAssignedPerDay[timeslot.day].count(lecture.courseCode) == 0)
                        {
                            // Add lecture to timetable
                            timetable.push_back(lecture);
                            timeslotOccupancy[lecture.timeslot].insert(lecture.branch);
                            lecturesAssignedPerDay[timeslot.day].insert(lecture.courseCode);

                            ++courseAssignments[course.code + branch];
                            ++assignedLectures;

                            break;
                        }
                    }
                    if (assignedLectures == requiredLectures)
                        break;
                }
                if (assignedLectures == requiredLectures)
                    break;
            }
        }

        // If required lectures are not assigned, assign to default branch
        if (assignedLectures < requiredLectures)
        {
            int remainingLectures = requiredLectures - assignedLectures;
            for (int i = 0; i < remainingLectures; ++i)
            {
                for (const auto &timeslot : timeslots)
                {
                    for (const auto &room : rooms)
                    {
                        Lecture lecture = {course.code, course.faculty, room.roomNumber, timeslot, course.batch, course.branches[0]};

                        if (isValidAssignment(lecture, timetable, timeslotOccupancy) &&
                            lecturesAssignedPerDay[timeslot.day].count(lecture.courseCode) == 0)
                        {
                            timetable.push_back(lecture);
                            timeslotOccupancy[lecture.timeslot].insert(lecture.branch);
                            lecturesAssignedPerDay[timeslot.day].insert(lecture.courseCode);

                            ++courseAssignments[course.code + course.branches[0]];
                            ++assignedLectures;

                            break;
                        }
                    }
                    if (assignedLectures == requiredLectures)
                        break;
                }
                if (assignedLectures == requiredLectures)
                    break;
            }
        }

        // Clear assigned lectures per day for next course
        for (int day = 1; day <= 5; ++day)
        {
            lecturesAssignedPerDay[day].clear();
        }
    }

    return timetable;  // Return generated timetable
}

// Function to generate CSV files for the timetable
void generateCSVFiles(const vector<Lecture> &timetable)
{
    // Map to store lectures by batch and branch
    map<pair<string, string>, vector<Lecture>> batchBranchLectures;

    // Group lectures by batch and branch
    for (const auto &lecture : timetable)
    {
        vector<string> branches;
        stringstream ss(lecture.branch);
        string branch;
        while (getline(ss, branch, ','))
        {
            branches.push_back(branch);
        }

        for (const auto &branch : branches)
        {
            batchBranchLectures[{lecture.batch, branch}].push_back(lecture);
        }
    }

    // Iterate over batches and branches
    for (const auto &entry : batchBranchLectures)
    {
        const auto &batchBranch = entry.first;
        const auto &lectures = entry.second;

        // Generate filename
        string filename = "timetable_" + batchBranch.first + "_" + batchBranch.second + ".csv";
        ofstream file(filename);

        // Write header to file
        file << "Slots,Monday,Tuesday,Wednesday,Thursday,Friday\n";

        // Write timetable data to file
        for (int slot = 1; slot <= 5; ++slot)
        {
            file << slot;

            for (int day = 1; day <= 5; ++day)
            {
                file << ",";

                auto it = find_if(lectures.begin(), lectures.end(), [=](const Lecture &l)
                                  { return l.timeslot.day == day && l.timeslot.slot == slot; });

                if (it != lectures.end())
                {
                    const Lecture &lecture = *it;
                    file << lecture.courseCode << "("
                         << lecture.faculty << "|"
                         << lecture.roomNumber << ")";
                }
                else
                {
                    file << " ";
                }
            }

            file << "\n";
        }
        file.close();
    }
}

// Function to generate CSV file for a specific faculty
void generateCSVFileForFaculty(const vector<Lecture> &timetable, const string &facultyName)
{
    vector<Lecture> facultyLectures;
    string facultyNameLower = facultyName;
    transform(facultyNameLower.begin(), facultyNameLower.end(), facultyNameLower.begin(), ::tolower);

    // Filter lectures for the specified faculty
    for (const auto &lecture : timetable)
    {
        string lectureFacultyLower = lecture.faculty;
        transform(lectureFacultyLower.begin(), lectureFacultyLower.end(), lectureFacultyLower.begin(), ::tolower);
        if (lectureFacultyLower == facultyNameLower)
        {
            facultyLectures.push_back(lecture);
        }
    }

    // If no lectures found for the faculty, return
    if (facultyLectures.empty())
    {
        cout << "No lectures found for faculty: " << facultyName << endl;
        return;
    }

    // Generate filename
    string filename = "timetable_" + facultyName + ".csv";
    ofstream file(filename);

    // Write header to file
    file << "Slots,Monday,Tuesday,Wednesday,Thursday,Friday\n";

    // Write timetable data to file
    for (int slot = 1; slot <= 5; ++slot)
    {
        file << slot;

        for (int day = 1; day <= 5; ++day)
        {
            file << ",";

            auto it = find_if(facultyLectures.begin(), facultyLectures.end(), [=](const Lecture &l)
                              { return l.timeslot.day == day && l.timeslot.slot == slot; });

            if (it != facultyLectures.end())
            {
                const Lecture &lecture = *it;
                file << lecture.courseCode << "("
                     << lecture.faculty << "|"
                     << lecture.roomNumber << ")";
            }
            else
            {
                file << " ";
            }
        }

        file << "\n";
    }
    file.close();
}

// Main function
int main()
{
    // Prompt user to enter semester type
    cout << "Enter semester type: (autumn/winter)" << endl;
    string semester;
    cin >> semester;

    // Validate semester type
    if (semester != "autumn" && semester != "winter")
    {
        cout << "Invalid semester type" << endl;
        return 1;
    }

    string inputFile = "data/" + semester + ".txt";  // Input file containing course data

    vector<Course> courses = readCoursesFromFile(inputFile);  // Read courses from file
    vector<Room> rooms = readRoomsFromFile("data/rooms.txt");  // Read rooms from file

    srand(static_cast<unsigned int>(time(0)));  // Seed random number generator

    // Generate timetable
    vector<Lecture> timetable = generateTimetable(courses, rooms);

    // Sort timetable by day and slot
    sort(timetable.begin(), timetable.end(), [](const Lecture &lhs, const Lecture &rhs)
         {
             if (lhs.timeslot.day == rhs.timeslot.day)
             {
                 return lhs.timeslot.slot < rhs.timeslot.slot;
             }
             return lhs.timeslot.day < rhs.timeslot.day; });

    // Generate CSV files for timetable
    generateCSVFiles(timetable);

    string facultyName;
    cout << "Enter faculty name to generate timetable CSV: ";
    cin >> facultyName;

    // Generate CSV file for specific faculty
    generateCSVFileForFaculty(timetable, facultyName);

    return 0;
}
