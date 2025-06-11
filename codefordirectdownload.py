import boto3

# Initialize the S3 client
s3 = boto3.client('s3')

def download_latest_zip(bucket_name, local_path):
    # List objects in the bucket
    response = s3.list_objects_v2(Bucket=bucket_name)
    if 'Contents' not in response:
        print("No files found in the bucket.")
        return

    # Find the latest uploaded ZIP file
    latest_file = max(
        [obj for obj in response['Contents'] if obj['Key'].endswith('.zip')],
        key=lambda x: x['LastModified'],
    )

    print(f"Downloading: {latest_file['Key']}")

    # Download the file
    s3.download_file(bucket_name, latest_file['Key'], local_path)
    print(f"File downloaded to: {local_path}")

# Provide your bucket name and local download path
bucket_name = 'testsample.192'
local_file_path = 'E:/final_file.zip'

# Call the function to download the latest ZIP
download_latest_zip(bucket_name, local_file_path)
