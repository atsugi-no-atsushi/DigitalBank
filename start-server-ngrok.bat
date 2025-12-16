@echo off
chcp 65001 > nul
title DigitalBank Server + ngrok

REM ===== 設定（ここだけ確認）=====
set "PROJECT_DIR=C:\Users\atsushi sato\Desktop\DigitalBank"
set "FIREBASE_KEY=C:\Users\atsushi sato\Desktop\DigitalBank\digitalbank-c5666-firebase-adminsdk-fbsvc-0a876e9b18.json"
set "PORT=3000"

echo =================================
echo DigitalBank 起動スクリプト
echo =================================

REM ===== フォルダ存在確認 =====
if not exist "%PROJECT_DIR%" (
  echo [ERROR] PROJECT_DIR が見つかりません:
  echo %PROJECT_DIR%
  pause
  exit /b 1
)

REM ===== Firebase Key 存在確認 =====
if not exist "%FIREBASE_KEY%" (
  echo [ERROR] Firebase Key JSON が見つかりません:
  echo %FIREBASE_KEY%
  echo.
  echo dir "%PROJECT_DIR%\*.json" を実行してファイル名を確認してください。
  pause
  exit /b 1
)

REM ===== 移動 =====
cd /d "%PROJECT_DIR%"

REM ===== 環境変数 =====
set "GOOGLE_APPLICATION_CREDENTIALS=%FIREBASE_KEY%"
echo Firebase Key:
echo %GOOGLE_APPLICATION_CREDENTIALS%
echo.

REM ===== server.js 起動 =====
echo Starting server.js ...
start "DigitalBank Server" cmd /k "node server.js"

REM ===== 起動待ち =====
timeout /t 3 > nul

REM ===== ngrok 起動（IPv4固定）=====
echo Starting ngrok ...
start "ngrok" cmd /k "ngrok http 127.0.0.1:%PORT%"

echo.
echo ================================
echo 起動完了
echo ngrok画面の https://****/web.html をスマホで開いてください
echo ================================
