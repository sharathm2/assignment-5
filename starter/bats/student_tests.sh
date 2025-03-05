#!/usr/bin/env bats

# File: student_tests.sh
# 
# Create your unit tests suit in this file

@test "Example: check ls runs without errors" {
    run ./dsh <<EOF                
ls
EOF

    # Assertions
    [ "$status" -eq 0 ]
}

# Basic command execution
@test "Basic command: ls" {
    run ./dsh <<EOF                
ls
EOF
    [ "$status" -eq 0 ]
}

# Simple pipe test
@test "Simple pipe: ls | grep c" {
    run ./dsh <<EOF
ls | grep c
EOF
    [ "$status" -eq 0 ]
    [[ "$output" =~ "dshlib.c" ]]
}

# Multiple pipe test
@test "Multiple pipes: ls | grep .c | sort" {
    run ./dsh <<EOF
ls | grep .c | sort
EOF
    [ "$status" -eq 0 ]
    # Check for sorted output of .c files
    [[ $(echo "$output" | grep ".c" | sort | tr -d '[:space:]') == $(echo "$output" | grep ".c" | tr -d '[:space:]') ]]
}

# Test built-in cd command
@test "Built-in command: cd" {
    run ./dsh <<EOF
cd ..
pwd
EOF
    [ "$status" -eq 0 ]
    # Should show parent directory
    [[ "$output" =~ "dsh3>" ]]
    [[ "$output" =~ "/starter/.." ]]
}

# Test cd with no arguments
@test "Built-in command: cd without arguments" {
    run ./dsh <<EOF
cd
pwd
EOF
    [ "$status" -eq 0 ]
    # Should not change directory
    [[ "$output" =~ "dsh3>" ]]
}

# Test exit command
@test "Exit command" {
    run ./dsh <<EOF
exit
EOF
    [ "$status" -eq 0 ]
    [[ "$output" =~ "exiting..." ]]
}

# Test quoted arguments
@test "Commands with quoted arguments" {
    run ./dsh <<EOF
echo "hello    world"
EOF
    [ "$status" -eq 0 ]
    [[ "$output" =~ "hello    world" ]]
}

# Test quoted arguments with pipe
@test "Pipe with quoted arguments" {
    run ./dsh <<EOF
echo "hello    world" | wc -w
EOF
    [ "$status" -eq 0 ]
    [[ "$output" =~ "2" ]]
}

# Test handling of whitespace
@test "Commands with extra whitespace" {
    run ./dsh <<EOF
   ls    |    grep    c   
EOF
    [ "$status" -eq 0 ]
    [[ "$output" =~ "dshlib.c" ]]
}

# Test non-existent command
@test "Error handling: non-existent command" {
    run ./dsh <<EOF
nonexistentcommand
EOF
    [ "$status" -eq 0 ]
    [[ "$output" =~ "execvp" ]]
}

# Test pipe with non-existent command
@test "Error handling: pipe with non-existent command" {
    run ./dsh <<EOF
ls | nonexistentcommand
EOF
    [ "$status" -eq 0 ]
    [[ "$output" =~ "execvp" ]]
}

# Test command that produces no output
@test "Command with no output piped" {
    run ./dsh <<EOF
echo -n "" | wc -c
EOF
    [ "$status" -eq 0 ]
    [[ "$output" =~ "0" ]]
}

# Test three commands in a pipeline
@test "Three commands in a pipeline" {
    run ./dsh <<EOF
ls | grep .c | wc -l
EOF
    [ "$status" -eq 0 ]
    # Number of .c files should be positive
    [[ $(echo "$output" | grep -o '[0-9]\+' | head -1) -gt 0 ]]
}

# Test complex pipeline
@test "Complex pipeline" {
    run ./dsh <<EOF
cat dshlib.c | grep int | head -5 | wc -l
EOF
    [ "$status" -eq 0 ]
    [[ $(echo "$output" | grep -o '[0-9]\+' | head -1) -eq 5 ]]
}
