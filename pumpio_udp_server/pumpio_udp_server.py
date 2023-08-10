# Pump it Up IO over Network for PIUTools
import socket
import threading
import input_handler
import output_handler


BIND_IP = "0.0.0.0"
PUMPIO_CONNECT_PORT = 15572
PUMPIO_INPUT_PORT = 15573
PUMPIO_OUTPUT_PORT = 15574

listener_reset = None



def client_connect_thread(server_ip,server_port):
    global listener_reset
    server_address = (server_ip, server_port)
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(server_address)
    while True:        
        data, client_address = sock.recvfrom(1)  # receive 1 byte of data just to know client address 
        print(f"[New Connection] Connected To Client: {client_address}!")
        listener_reset = True


def input_server_thread(server_ip, server_port):
    global listener_reset
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((server_ip, server_port))
 
    last_state = b""
    while listener_reset is None:
        pass
    while True :
        # If a new client connects, we're going to initiate the handshake again.
        if(listener_reset is True):
            print("Resetting Input Server Connection State!")
            listener_reset = False
            data, client_address = sock.recvfrom(1)
            last_state = b""

        state = input_handler.get_input_state()        
        if state != last_state:            
            # Send new state to client
            sock.sendto(state, client_address)
            last_state = state

def output_server_thread(server_ip, server_port):  
    global listener_reset
    while listener_reset is None:
        pass    
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((server_ip, server_port))

    while True:
        data = sock.recv(8)
        if(len(data) == 8):
            output_handler.process_output(data)

            

if __name__ == "__main__":
    print("[PUMPIO_NET] Starting Service")
    threading.Thread(target=client_connect_thread, args=(BIND_IP, PUMPIO_CONNECT_PORT), daemon=True).start()
    threading.Thread(target=input_server_thread, args=(BIND_IP, PUMPIO_INPUT_PORT), daemon=True).start()
    threading.Thread(target=output_server_thread, args=(BIND_IP, PUMPIO_OUTPUT_PORT), daemon=True).start()
    x = input("Press Any key to exit")

