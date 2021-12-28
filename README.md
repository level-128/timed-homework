# COMP1038 coursework 2: Food Order Management System.

---

This code is my coursework for University of Nottingham Ningbo COMP1038 (Programming and Algorithms) when I was
an exchange student studying in PRC. 

Some students were curious about how did I implement this coursework -- which motivates me to free this code.
The requirement of this coursework is to implement a Food Order Management System using C programing language
(C99 standard with VLA, POSIX environment), which users could select 
and order food in pre-recorded categories, and program could read and modify csv sheets which stores the
food menu and transition history. The program should be able to modify categories and food items, view transition
history in a password protected Admin session.

I implemented few basic data structures:

- optimized single link list to store the categories and items. 
- Unicode string (mutable) extended from link list with basic methods.
- Key-Value pair Hash table
- Table structure which could search sub-sheets or columns using key or index.
- A tiny runtime to perform dynamic type-check for values in .csv file
- A .csv file parser and reader, which allows storing different sheets in a single .csv file or read a sheet
which locates in multiple .csv files.

___COMP1038 Is an intro course for CS students, so this code should be very easy to understand.___

## license:

This work is dual licensed under GNU Affero General Public License version 3 and Mozilla Public
License version 2.0 with Exception:

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Affero General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Affero General Public License for more details.

	You should have received a copy of the GNU Affero General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.


	This Source Code Form is subject to the terms of the Mozilla Public License,
	v. 2.0. If a copy of the MPL was not distributed with this file, You can
	obtain one at https://mozilla.org/MPL/2.0/.

    Copyright Exception:
    Modifying this program without accepting GNU Affero General Public License
    version 3 and Mozilla Public License version 2.0 is allowed only for
    coursework evaluating purpose; Program, which combined with this work, by
    this purpose could remain proprietary or limit rights.

## compile:
In University of Nottingham Ningbo, courseworks are evaluated on an CentOS 7 server running GCC 9.x. 
The program should compile with no warnings and errors with following params:

    gcc -std=c99 -lm –Wall -g -fsanitize=leak FoodOrderMgntSystem.c -o FoodOrderMgntSystem

for this work, as long as a GNU/Linux environment is used, you could compile either using GCC or Clang, 
with or without optimization. For example:

    gcc -O3 FoodOrderMgntSystem.c
    ./a.out

---

Feel free to ask me questions :)