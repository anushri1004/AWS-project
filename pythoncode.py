import serial
import os
import re

def receive_zip_file(port, baudrate, output_path):
    output_dir = os.path.dirname(output_path)

    # Ensure the output directory exists
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
                else:
                    # Skip any other irrelevant messages
                    print(f"Skipping irrelevant message: '{line}'")

            if file_size is None:
                print("Could not determine file size. Exiting.")
                return

            bytes_received = 0
            print("Starting file reception...")

            # Receive the actual file data
            while bytes_received < file_size:
                if ser.in_waiting > 0:
                    # Read only up to the remaining bytes to avoid receiving extra data
                    remaining_bytes = file_size - bytes_received
                    data = ser.read(min(1024, remaining_bytes))
                    output_file.write(data)
                    bytes_received += len(data)
                    print(f"Received: {bytes_received}/{file_size} bytes")

            print(f"File received successfully and saved to {output_path}")

            # Ensure the file is properly closed before verifying
            output_file.close()

            # Verify file size after transfer
            received_size = os.path.getsize(output_path)
            if received_size == file_size:
                print(f"Transfer complete! File size matches: {received_size} bytes")
            else:
                print(f"Transfer incomplete! Expected: {file_size} bytes, but got {received_size} bytes")

            # Display the total file size transferred
            print(f"Total bytes transferred: {received_size} bytes")

    except serial.SerialException as e:
        print(f"Serial communication error: {e}")
    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    serial_port = 'COM11'  # Adjust as needed
    baudrate = 115200
    output_zip_path = 'E:\\file_received.zip'  # Path to save the ZIP file

    # Call function to receive the ZIP file via serial
    receive_zip_file(serial_port, baudrate, output_zip_path)
