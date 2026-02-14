@echo off
setlocal EnableExtensions

set "SCRIPT_DIR=%~dp0"
set "NRO_FILE=%SCRIPT_DIR%happyfoil.nro"
set "OUTPUT_ZIP=%SCRIPT_DIR%happyfoil.zip"
set "STAGE_DIR=%SCRIPT_DIR%release_zip_stage"
set "TARGET_DIR=%STAGE_DIR%\switch\HappyFoil"

if not exist "%NRO_FILE%" (
  echo ERROR: Missing NRO file: "%NRO_FILE%"
  echo Build first, then run this script again.
  exit /b 1
)

if exist "%STAGE_DIR%" rmdir /s /q "%STAGE_DIR%"
mkdir "%TARGET_DIR%" || (
  echo ERROR: Failed to create staging directory.
  exit /b 1
)

copy /y "%NRO_FILE%" "%TARGET_DIR%\happyfoil.nro" >nul || (
  echo ERROR: Failed to copy happyfoil.nro into staging directory.
  exit /b 1
)

if exist "%OUTPUT_ZIP%" del /f /q "%OUTPUT_ZIP%"

powershell -NoProfile -ExecutionPolicy Bypass -Command "Compress-Archive -Path '%STAGE_DIR%\switch' -DestinationPath '%OUTPUT_ZIP%' -Force"
if errorlevel 1 (
  echo ERROR: Failed to create "%OUTPUT_ZIP%".
  if exist "%STAGE_DIR%" rmdir /s /q "%STAGE_DIR%"
  exit /b 1
)

if exist "%STAGE_DIR%" rmdir /s /q "%STAGE_DIR%"

echo Release package created: "%OUTPUT_ZIP%"
echo Zip structure: switch\HappyFoil\happyfoil.nro
exit /b 0
