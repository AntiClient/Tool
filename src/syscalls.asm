.code

NtReadVirtualMemory proc
	mov r10, rcx
	mov eax, 3Fh
	syscall
	ret
NtReadVirtualMemory endp

NtQueryVirtualMemory proc
	mov r10, rcx
	mov eax, 23h
	syscall
	ret
NtQueryVirtualMemory endp

end