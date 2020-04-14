@echo off

REM Copyright (c) 2019 Nader G. Zeid
REM
REM This file is part of Memorypa.
REM
REM This program is free software: you can redistribute it and/or modify
REM it under the terms of the GNU General Public License as published by
REM the Free Software Foundation, either version 3 of the License, or
REM (at your option) any later version.
REM
REM This program is distributed in the hope that it will be useful,
REM but WITHOUT ANY WARRANTY; without even the implied warranty of
REM MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
REM GNU General Public License for more details.
REM
REM You should have received a copy of the GNU General Public License
REM along with Memorypa. If not, see <https://www.gnu.org/licenses/gpl.html>.


FOR /F "tokens=*" %%g IN ('cd') DO (SET current_directory=%%~nxg)
IF %current_directory:~0,8% NEQ memorypa (
	echo Must be executed from the root Memorypa directory!
	goto:eof
)
IF NOT EXIST .\win mkdir .\win
cl /LD /MT /W4 /sdl /O2 /Fo".\win\memorypa.obj" /I include src\memorypa.c /link /DEF:".\src\memorypa.def" /IMPLIB:".\win\memorypa.lib" /OUT:".\win\memorypa.dll"
cl /LD /MT /W4 /sdl /O2 /Fo".\win\memorypa_overrider.obj" /I include src\memorypa_overrider.c /link /DEF:".\src\memorypa_overrider.def" /IMPLIB:".\win\memorypa_overrider.lib" /OUT:".\win\memorypa_overrider.dll" ".\win\memorypa.lib"
cl /LD /MT /W4 /sdl /O2 /Fo".\win\memorypa_aligned_overrider.obj" /I include src\memorypa_aligned_overrider.c /link /DEF:".\src\memorypa_overrider.def" /IMPLIB:".\win\memorypa_aligned_overrider.lib" /OUT:".\win\memorypa_aligned_overrider.dll" ".\win\memorypa.lib"
cl /MT /W4 /sdl /O2 /Fo".\win\test_memorypa.obj" /I include src\test_memorypa.c /link /OUT:".\win\test_memorypa.exe" ".\win\memorypa.lib"
cl /MT /W4 /sdl /O2 /Fo".\win\test_memorypa_rescue.obj" /I include /D MEMORYPA_TEST_RESCUE src\test_memorypa.c /link /OUT:".\win\test_memorypa_rescue.exe" ".\win\memorypa.lib"
cl /MT /W4 /sdl /O2 /Fo".\win\benchmark.obj" /I include src\benchmark.c /link /OUT:".\win\benchmark.exe"
cl /MT /W4 /sdl /O2 /Fo".\win\benchmark_memorypa.obj" /I include src\benchmark_memorypa.c /link /OUT:".\win\benchmark_memorypa.exe" ".\win\memorypa.lib"
cl /MT /W4 /sdl /O2 /Fo".\win\example_standard.obj" /I include src\example_standard.c /link /OUT:".\win\example_standard.exe" ".\win\memorypa.lib"
cl /MT /W4 /sdl /O2 /Fo".\win\example_profiler.obj" /I include src\example_profiler.c /link /OUT:".\win\example_profiler.exe" ".\win\memorypa.lib"
cl /MT /W4 /sdl /O2 /Fo".\win\example_with_overriding.obj" /I include src\example_with_overriding.c /link /OUT:".\win\example_with_overriding.exe" ".\win\memorypa_overrider.lib"
cl /MT /W4 /sdl /O2 /Fo".\win\example_with_overriding_and_alignment.obj" /I include src\example_with_overriding_and_alignment.c /link /OUT:".\win\example_with_overriding_and_alignment.exe" ".\win\memorypa_aligned_overrider.lib"
