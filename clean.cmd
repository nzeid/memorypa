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
FOR /F "tokens=*" %%g IN ('dir /s /b ".\win"') DO echo del "%%g" && del /Q "%%g"
echo rmdir .\win && rmdir .\win
