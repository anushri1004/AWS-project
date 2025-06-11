import serial
import os
import re
import time

def is_irrelevant_message(line):

    irrelevant_keywords = [
        'load:', 'Connecting to WiFi', 'Connected to WiFi',
        'Response:', 'Latest file found:', 'File URL:', 'File saved to SD card successfully'
    ]
    
    
    return any(keyword in line for keyword in irrelevant_keywords)

def receive_zip_file(port, baudrate, output_path):
    output_dir = os.path.dirname(output_path)

    
    if not os.path.exists(output_dir):
        print(f"Directory does not exist: {output_dir}")
        return

    try:
        with serial.Serial(port, baudrate, timeout=1) as ser, open(output_path, 'wb') as output_file:
            print("Waiting for file size...")

            file_size = None
            while file_size is None:
                line = ser.readline().decode(errors='ignore').strip()

                # Check for the file size message, skip irrelevant ESP32 messages
                if "File size:" in line:
                    match = re.search(r'File size: (\d+)', line)
                    if match:
                        file_size = int(match.group(1))
                        print(f"File size to receive: {file_size} bytes")
                elif not is_irrelevant_message(line):
                    print(f"Received message: '{line}'")  # Print any relevant messages

            if file_size is None:
                print("Could not determine file size. Exiting.")
                return

            bytes_received = 0
            print("Starting file reception...")

            # Receive the actual file data
            start_time = time.time()  # Start timer
            while bytes_received < file_size:
                if ser.in_waiting > 0:
                    remaining_bytes = file_size - bytes_received
                    data = ser.read(min(1024, remaining_bytes))
                    output_file.write(data)
                    output_file.flush()  # Ensure data is written to disk
                    bytes_received += len(data)
                    print(f"Received: {bytes_received}/{file_size} bytes")
                else:
                    time.sleep(0.1)  # If no data is waiting, wait briefly

            end_time = time.time()  # End timer
            print(f"File received successfully and saved to {output_path}")

            # Verify file size after transfer
            bytes_received = os.path.getsize(output_path)
            if bytes_received == file_size:
                print(f"Transfer complete! File size matches: {bytes_received} bytes")
            else:
                print(f"Transfer incomplete! Expected: {file_size} bytes, but got {bytes_received} bytes")

            # Display the total file size transferred
            print(f"Total bytes transferred: {bytes_received} bytes")
            print(f"Time taken for transfer: {end_time - start_time:.2f} seconds")

    except serial.SerialException as e:
        print(f"Serial communication error: {e}")
    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    serial_port = 'COM11'  # Adjust as needed
    baudrate = 115200
    output_zip_path = 'E:\\final.zip'  # Path to save the ZIP file

    # Call function to receive the ZIP file via serial
    receive_zip_file(serial_port, baudrate, output_zip_path)
