@echo off
REM ===== DigitalBank Server Starter =====

REM フォルダに移動
cd /d "C:\Users\atsushi sato\Desktop\DigitalBank"

REM Firebase サービスアカウントキーを設定
set "GOOGLE_APPLICATION_CREDENTIALS=C:\Users\atsushi sato\Desktop\DigitalBank\digitalbank-c5666-firebase-adminsdk-fbsvc-0a876e9b18.json"

REM 確認表示
echo GOOGLE_APPLICATION_CREDENTIALS=%GOOGLE_APPLICATION_CREDENTIALS%
echo.

REM server.js 起動
node server.js

REM 終了防止（エラー時に画面を見るため）
pause
