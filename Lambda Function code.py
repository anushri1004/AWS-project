import boto3
import json

def lambda_handler(event, context):
    s3 = boto3.client('s3')
    bucket_name = 'your-s3-bucket-name'  # Replace with your bucket name
    
    # Get list of all files in the bucket
    response = s3.list_objects_v2(Bucket=bucket_name)
    files = response.get('Contents', [])

    if not files:
        return {
            'statusCode': 404,
            'body': json.dumps({'error': 'No files found in the bucket'})
        }

    # Find the latest file by last modified date
    latest_file = max(files, key=lambda x: x['LastModified'])

    # Generate a pre-signed URL for the latest ZIP file
    file_url = s3.generate_presigned_url(
        'get_object',
        Params={'Bucket': bucket_name, 'Key': latest_file['Key']},
        ExpiresIn=3600
    )

    return {
        'statusCode': 200,
        'body': json.dumps({'file_url': file_url})
    }
