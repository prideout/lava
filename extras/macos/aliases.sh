alias  init="cmake -H. -B.debug -GXcode"
alias build="cmake --build .debug"
alias   run="open .debug/Debug/lavamac.app"
alias debug="lldb .debug/Debug/lavamac.app/Contents/MacOS/lavamac -o run"
