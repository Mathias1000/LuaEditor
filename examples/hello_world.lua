--------------------------------------------------------------------------------
-- Klassendefinitionen und Basisklassen
--------------------------------------------------------------------------------

-- Basisklasse Person: Allgemeine Eigenschaften und Methoden für Menschen
Person = {}
Person.__index = Person

-- Konstruktor für Person
function Person:new(name, age)
    local self = setmetatable({}, Person)
    self.name = name or "Unknown"
    self.age = age or 0
    self.gender = "Unspecified"
    self.address = "Unknown"
    self.phone = "N/A"
    return self
end

-- Begrüßung
function Person:greet()
    print("Hello, my name is " .. self.name)
end

-- Alter ausgeben
function Person:getAge()
    return self.age
end

-- Geschlecht setzen
function Person:setGender(gender)
    self.gender = gender
end

-- Adresse setzen
function Person:setAddress(address)
    self.address = address
end

-- Telefonnummer setzen
function Person:setPhone(phone)
    self.phone = phone
end

-- Vollständige Info ausgeben
function Person:printInfo()
    print("Person Info:")
    print(" Name: " .. self.name)
    print(" Age: " .. self.age)
    print(" Gender: " .. self.gender)
    print(" Address: " .. self.address)
    print(" Phone: " .. self.phone)
end

-- Prüft, ob Person volljährig ist
function Person:isAdult()
    return self.age >= 18
end


--------------------------------------------------------------------------------
-- Klasse Student: Erbt von Person, enthält Student-spezifische Eigenschaften
--------------------------------------------------------------------------------

Student = setmetatable({}, {__index = Person})
Student.__index = Student

-- Konstruktor
function Student:new(name, age, studentID)
    local self = setmetatable(Person:new(name, age), Student)
    self.studentID = studentID or "0000"
    self.courses = {}
    self.grades = {}
    self.status = "active" -- Status: active, suspended, graduated
    return self
end

-- Einschreiben in Kurs
function Student:enroll(course)
    for _, c in ipairs(self.courses) do
        if c == course then
            print(self.name .. " is already enrolled in " .. course.name)
            return
        end
    end
    table.insert(self.courses, course)
    course:addStudent(self) -- Rückverweis im Kurs hinzufügen
    print(self.name .. " enrolled in " .. course.name)
end

-- Noten setzen
function Student:setGrade(course, grade)
    if type(grade) ~= "number" or grade < 0 or grade > 100 then
        print("Invalid grade value for " .. self.name)
        return
    end
    self.grades[course.code] = grade
    print(self.name .. "'s grade for " .. course.name .. " set to " .. grade)
end

-- Durchschnittsnote berechnen
function Student:getAverageGrade()
    local sum = 0
    local count = 0
    for _, grade in pairs(self.grades) do
        sum = sum + grade
        count = count + 1
    end
    if count == 0 then return nil end
    return sum / count
end

-- Kursliste ausgeben
function Student:listCourses()
    print(self.name .. "'s courses:")
    for i, c in ipairs(self.courses) do
        print("  - " .. c.name .. " (" .. c.code .. ")")
    end
end

-- Status setzen
function Student:setStatus(status)
    local validStatuses = {active=true, suspended=true, graduated=true}
    if validStatuses[status] then
        self.status = status
        print(self.name .. "'s status set to " .. status)
    else
        print("Invalid status: " .. status)
    end
end

-- Status abfragen
function Student:getStatus()
    return self.status
end

-- Details ausgeben
function Student:printInfo()
    Person.printInfo(self)
    print(" Student ID: " .. self.studentID)
    print(" Status: " .. self.status)
    local avg = self:getAverageGrade()
    if avg then
        print(string.format(" Average Grade: %.2f", avg))
    else
        print(" No grades recorded.")
    end
end


--------------------------------------------------------------------------------
-- Klasse Teacher: Erbt von Person, mit Kurs-Verwaltung und Fachgebiet
--------------------------------------------------------------------------------

Teacher = setmetatable({}, {__index = Person})
Teacher.__index = Teacher

function Teacher:new(name, age, employeeID)
    local self = setmetatable(Person:new(name, age), Teacher)
    self.employeeID = employeeID or "T000"
    self.coursesTaught = {}
    self.subjects = {}
    self.salary = 0
    return self
end

function Teacher:addCourse(course)
    for _, c in ipairs(self.coursesTaught) do
        if c == course then
            print(self.name .. " already teaches " .. course.name)
            return
        end
    end
    table.insert(self.coursesTaught, course)
    course:setTeacher(self)
    print(self.name .. " now teaches " .. course.name)
end

function Teacher:removeCourse(course)
    for i, c in ipairs(self.coursesTaught) do
        if c == course then
            table.remove(self.coursesTaught, i)
            course.teacher = nil
            print(self.name .. " no longer teaches " .. course.name)
            return
        end
    end
    print(self.name .. " does not teach " .. course.name)
end

function Teacher:addSubject(subject)
    for _, s in ipairs(self.subjects) do
        if s == subject then
            print(self.name .. " already has subject " .. subject)
            return
        end
    end
    table.insert(self.subjects, subject)
    print(self.name .. " added subject " .. subject)
end

function Teacher:listSubjects()
    print(self.name .. "'s subjects:")
    for _, s in ipairs(self.subjects) do
        print("  - " .. s)
    end
end

function Teacher:setSalary(amount)
    if amount < 0 then
        print("Salary cannot be negative")
        return
    end
    self.salary = amount
    print(self.name .. "'s salary set to " .. amount)
end

function Teacher:getSalary()
    return self.salary
end

function Teacher:listCourses()
    print(self.name .. "'s courses taught:")
    for _, c in ipairs(self.coursesTaught) do
        print("  - " .. c.name .. " (" .. c.code .. ")")
    end
end

function Teacher:printInfo()
    Person.printInfo(self)
    print(" Employee ID: " .. self.employeeID)
    print(" Salary: " .. self.salary)
    self:listSubjects()
    self:listCourses()
end


--------------------------------------------------------------------------------
-- Klasse Course: Enthält Studenten, Lehrer, Prüfungsdaten
--------------------------------------------------------------------------------

Course = {}
Course.__index = Course

function Course:new(name, code)
    local self = setmetatable({}, Course)
    self.name = name or "Unknown Course"
    self.code = code or "XXX000"
    self.students = {}
    self.teacher = nil
    self.credits = 3
    self.schedule = {}
    self.room = "TBD"
    return self
end

function Course:addStudent(student)
    for _, s in ipairs(self.students) do
        if s == student then
            print(student.name .. " is already enrolled in " .. self.name)
            return
        end
    end
    table.insert(self.students, student)
    print(student.name .. " added to course " .. self.name)
end

function Course:removeStudent(student)
    for i, s in ipairs(self.students) do
        if s == student then
            table.remove(self.students, i)
            print(student.name .. " removed from course " .. self.name)
            return
        end
    end
    print(student.name .. " not found in course " .. self.name)
end

function Course:setTeacher(teacher)
    if self.teacher then
        print("Course " .. self.name .. " already has a teacher: " .. self.teacher.name)
        return
    end
    self.teacher = teacher
    teacher:addCourse(self)
    print("Teacher " .. teacher.name .. " assigned to course " .. self.name)
end

function Course:setCredits(credits)
    if credits <= 0 then
        print("Credits must be positive")
        return
    end
    self.credits = credits
end

function Course:setRoom(room)
    self.room = room
end

function Course:addSchedule(day, startHour, duration)
    table.insert(self.schedule, {day=day, start=startHour, duration=duration})
end

function Course:listSchedule()
    print("Schedule for " .. self.name .. ":")
    for _, slot in ipairs(self.schedule) do
        print("  " .. slot.day .. " from " .. slot.start .. " for " .. slot.duration .. " hours")
    end
end

function Course:listParticipants()
    print("Course " .. self.name .. " participants:")
    if self.teacher then
        print(" Teacher: " .. self.teacher.name)
    else
        print(" No teacher assigned")
    end
    print(" Students:")
    for _, s in ipairs(self.students) do
        print("  - " .. s.name .. " (ID: " .. s.studentID .. ")")
    end
end

function Course:printInfo()
    print("Course Info:")
    print(" Name: " .. self.name)
    print(" Code: " .. self.code)
    print(" Credits: " .. self.credits)
    print(" Room: " .. self.room)
    self:listSchedule()
    self:listParticipants()
end


--------------------------------------------------------------------------------
-- Klasse School: Verwaltung von Schülern, Lehrern und Kursen
--------------------------------------------------------------------------------

School = {}
School.__index = School

function School:new(name)
    local self = setmetatable({}, School)
    self.name = name or "Unnamed School"
    self.students = {}
    self.teachers = {}
    self.courses = {}
    return self
end

function School:addStudent(student)
    for _, s in ipairs(self.students) do
        if s == student then
            print("Student " .. student.name .. " already in school")
            return
        end
    end
    table.insert(self.students, student)
    print("Added student: " .. student.name)
end



function School:removeStudent(student)
    for i, s in ipairs(self.students) do
        if s == student then
            table.remove(self.students, i)
            print("Removed student: " .. student.name)
            return
        end
    end
    print("Student " .. student.name .. " not found in school")
end

function School:addTeacher(teacher)
    for _, t in ipairs(self.teachers) do
        if t == teacher then
            print("Teacher " .. teacher.name .. " already in school")
            return
        end
    end
    table.insert(self.teachers, teacher)
    print("Added teacher: " .. teacher.name)
end

function School:removeTeacher(teacher)
    for i, t in ipairs(self.teachers) do
        if t == teacher then
            table.remove(self.teachers, i)
            print("Removed teacher: " .. teacher.name)
            return
        end
    end
    print("Teacher " .. teacher.name .. " not found in school")
end

function School:addCourse(course)
    for _, c in ipairs(self.courses) do
        if c == course then
            print("Course " .. course.name .. " already in school")
            return
        end
    end
    table.insert(self.courses, course)
    print("Added course: " .. course.name)
end

function School:removeCourse(course)
    for i, c in ipairs(self.courses) do
        if c == course then
            table.remove(self.courses, i)
            print("Removed course: " .. course.name)
            return
        end
    end
    print("Course " .. course.name .. " not found in school")
end

function School:findStudentByName(name)
    for _, s in ipairs(self.students) do
        if s.name == name then
            return s
        end
    end
    return nil
end

function School:findTeacherByName(name)
    for _, t in ipairs(self.teachers) do
        if t.name == name then
            return t
        end
    end
    return nil
end

function School:findCourseByCode(code)
    for _, c in ipairs(self.courses) do
        if c.code == code then
            return c
        end
    end
    return nil
end

function School:listAll()
    print("School: " .. self.name)
    print("Teachers:")
    for _, t in ipairs(self.teachers) do
        print(" - " .. t.name)
    end
    print("Students:")
    for _, s in ipairs(self.students) do
        print(" - " .. s.name)
    end
    print("Courses:")
    for _, c in ipairs(self.courses) do
        print(" - " .. c.name)
    end
end

function School:printInfo()
    print("---- School Info ----")
    print("Name: " .. self.name)
    print("Number of Teachers: " .. #self.teachers)
    print("Number of Students: " .. #self.students)
    print("Number of Courses: " .. #self.courses)
end


--------------------------------------------------------------------------------
-- Hilfsfunktionen und Simulation
--------------------------------------------------------------------------------

-- Hilfsfunktion: Generiert zufälligen Namen (einfach)
local function randomName()
    local firstNames = {"Alice", "Bob", "Charlie", "Diana", "Eve", "Frank", "Grace", "Heidi", "Ivan", "Judy"}
    local lastNames = {"Smith", "Johnson", "Williams", "Jones", "Brown", "Davis", "Miller", "Wilson"}
    return firstNames[math.random(#firstNames)] .. " " .. lastNames[math.random(#lastNames)]
end

-- Hilfsfunktion: Zufälliges Alter zwischen 16 und 30
local function randomAge()
    return math.random(16, 30)
end

-- Hilfsfunktion: Zufällige Noten generieren
local function randomGrade()
    return math.random(50, 100)
end

--------------------------------------------------------------------------------
-- Simulation: Schule füllen und Aktionen durchführen
--------------------------------------------------------------------------------

-- Initialisiere Zufallszahlengenerator
math.randomseed(os.time())

local mySchool = School:new("Tech High School")

-- Erzeuge Lehrer
for i = 1, 5 do
    local name = randomName()
    local age = randomAge() + 10
    local teacher = Teacher:new(name, age, "T" .. 100 + i)
    teacher:setSalary(3000 + math.random(0, 2000))
    teacher:addSubject("Subject " .. i)
    mySchool:addTeacher(teacher)
end

-- Erzeuge Kurse
for i = 1, 10 do
    local course = Course:new("Course " .. i, "C" .. 100 + i)
    course:setCredits(3 + (i % 3))
    course:setRoom("Room " .. (100 + i))
    course:addSchedule("Monday", 8 + (i % 3), 2)
    course:addSchedule("Wednesday", 10 + (i % 2), 2)
    mySchool:addCourse(course)
end

-- Erzeuge Studenten
for i = 1, 20 do
    local name = randomName()
    local age = randomAge()
    local student = Student:new(name, age, "S" .. 1000 + i)
    if i % 2 == 0 then
        student:setStatus("active")
    else
        student:setStatus("suspended")
    end
    mySchool:addStudent(student)
end

-- Lehrer den Kursen zuweisen
for i, course in ipairs(mySchool.courses) do
    local teacher = mySchool.teachers[(i % #mySchool.teachers) + 1]
    course:setTeacher(teacher)
end

-- Studenten in Kurse einschreiben und Noten setzen
for _, student in ipairs(mySchool.students) do
    for i = 1, 3 do
        local course = mySchool.courses[math.random(#mySchool.courses)]
        student:enroll(course)
        student:setGrade(course, randomGrade())
    end
end

-- Ausgabe einiger Infos
mySchool:printInfo()
mySchool:listAll()

for _, student in ipairs(mySchool.students) do
    student:printInfo()
    print()
end

for _, teacher in ipairs(mySchool.teachers) do
    teacher:printInfo()
    print()
end

for _, course in ipairs(mySchool.courses) do
    course:printInfo()
    print()
end

