#!/bin/bash
# 用法：./push.sh              （執行後輸入 commit 訊息）
#       ./push.sh "訊息內容"   （直接帶訊息）
set -e

# 不管從哪個目錄執行，都切到 repo 根目錄
cd "$(git -C "$(dirname "$0")" rev-parse --show-toplevel)"

msg="$*"
if [ -z "$msg" ]; then
    read -r -p "Commit message: " msg
fi
if [ -z "$msg" ]; then
    echo "沒有輸入 commit 訊息，取消"
    exit 1
fi

git add -A
if git diff --cached --quiet; then
    echo "沒有變更需要 commit"
else
    git status --short
    git commit -m "$msg"
fi

git push
