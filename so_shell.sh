#!/bin/bash

HISTORY_SIZE=100
history=()
history_count=0

add_to_history() {
    local command="$1"
    if [ $history_count -lt $HISTORY_SIZE ]; then
        history[$history_count]="$command"
        ((history_count++))
    else
        for ((i=0; i<HISTORY_SIZE-1; i++)); do
            history[$i]="${history[$((i+1))]}"
        done
        history[$((HISTORY_SIZE-1))]="$command"
    fi
}

print_history() {
    for ((i=0; i<history_count; i++)); do
        echo "$((i+1)): ${history[$i]}"
    done
}

execute_command() {
    local cmdline="$1"
    local background=0
    local input_file=""
    local output_file=""
    local pipe_index=-1
    local args=()
    
    add_to_history "$cmdline"

    IFS=' ' read -r -a args <<< "$cmdline"

    for i in "${!args[@]}"; do
        case "${args[$i]}" in
            "&")
                background=1
                unset 'args[$i]'
                ;;
            "<")
                input_file="${args[$((i+1))]}"
                unset 'args[$i]' 'args[$((i+1))]'
                ;;
            ">")
                output_file="${args[$((i+1))]}"
                unset 'args[$i]' 'args[$((i+1))]'
                ;;
            "|")
                pipe_index=$i
                args[$i]=""
                ;;
        esac
    done

    # Configuração do comando de entrada/saída
    if [ -n "$input_file" ]; then
        exec 3<&0
        exec < "$input_file"
    fi

    if [ -n "$output_file" ]; then
        exec 4>&1
        exec > "$output_file"
    fi

    # Executando o comando
    if [ $pipe_index -ge 0 ]; then
        ${args[0]} | ${args[$((pipe_index+1))]}
    else
        "${args[@]}"
    fi

    if [ $background -eq 0 ]; then
        wait
    fi
}

while true; do
    echo -n "shell> "
    read -r cmdline

    if [ "$cmdline" == "exit" ]; then
        break
    elif [ "$cmdline" == "history" ]; then
        print_history
    else
        execute_command "$cmdline"
    fi
done
