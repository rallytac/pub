# Engage Provisioning Tool (EPT)

A secure provisioning tool for Rally Tactical Systems' Engage platform. EPT encrypts and packages Engage provisioning profiles, making them secure for distribution and deployment.

## What is EPT?

The Engage Provisioning Tool (EPT) is a Python utility that:
- **Encrypts** Engage provisioning profiles with AES-256 encryption
- **Packages** multiple configuration files into a single encrypted profile
- **Decrypts** and extracts provisioning profiles for deployment
- **Manages** identities, policies, missions, apps, and certificate stores

## Prerequisites

- Python 3.6 or higher
- `cryptography` library

### Installation

```bash
pip install cryptography
```

## Usage

### Basic Syntax

```bash
python3 eptool.py -pwd <password> -i <input_file> -o <output_file> [options]
```

### Packing (Encrypting) a Provisioning Profile

**Basic packing:**
```bash
python3 eptool.py -pwd mypassword -i profile.json -o profile.ep
```

**Pack with custom identity:**
```bash
python3 eptool.py -pwd mypassword -i profile.json -o profile.ep -identity custom_identity.json
```

**Pack without compression (default is packed):**
```bash
python3 eptool.py --no-pack -pwd mypassword -i profile.json -o profile.ep
```

### Unpacking (Decrypting) a Provisioning Profile

```bash
python3 eptool.py -pwd mypassword -i profile.ep -o profile.json
```

## Command Line Options

### Required Arguments
- **`-pwd password`**: Password for encrypting/decrypting the provisioning file
- **`-i input_file`**: Input file (JSON for packing, .ep for unpacking)
- **`-o output_file`**: Output file (.ep for packing, JSON for unpacking)

### Optional Arguments
- **`-identity identity_file`**: Override identity file during packing
- **`--pack`**: Enable packing mode (default)
- **`--no-pack`**: Disable packing mode (unpack/decrypt)

## Provisioning Profile Structure

### Input JSON Format

```json
{
  "engageProvisioning": {
    "identities": [
      "@file://identity.json",
      "@as://engage.identity:custom_identity.json"
    ],
    "policies": [
      "@file://policy.json"
    ],
    "missions": [
      "@file://mission.json"
    ],
    "apps": [
      "@file://app.json"
    ],
    "certStores": [
      "@file://certificates.certstore"
    ]
  }
}
```

### File References

EPT supports several file reference formats:

- **`@file://path/to/file`**: Reference to local file
- **`@as://engage.identity:filename`**: Special identity reference
- **Direct JSON content**: Inline configuration objects

## Encryption Details

### Security Features
- **AES-256-CBC** encryption
- **PBKDF2-HMAC-SHA256** key derivation (10,000 iterations)
- **Random IV** generation for each encryption
- **PKCS7 padding** for data alignment
- **Base64 encoding** for safe transport

### Key Derivation
```python
# Key is derived using:
key = PBKDF2(password, salt, iterations=10000, key_length=32)
salt = '04DBAA5900D5421D9CCB25A54ED4FA56'
```

## File Processing

### During Packing
EPT processes different file types:

1. **Policies**: JSON files embedded as objects
2. **Identities**: JSON files with special container handling
3. **Missions**: JSON files embedded as objects
4. **Apps**: JSON files embedded as objects
5. **Certificate Stores**: Binary files encoded as Base64

### File Embedding
```json
// Original reference
"policies": ["@file://my_policy.json"]

// Becomes embedded
"policies": [{
  "container": "my_policy.json",
  "content": { /* policy JSON content */ }
}]
```

## Examples

### Creating a Basic Provisioning Profile

**1. Create input JSON:**
```json
{
  "engageProvisioning": {
    "identities": ["@file://default_identity.json"],
    "policies": ["@file://security_policy.json"],
    "missions": ["@file://tactical_mission.json"]
  }
}
```

**2. Pack the profile:**
```bash
python3 eptool.py -pwd secure123 -i profile.json -o tactical_profile.ep
```

**3. Deploy the encrypted profile:**
```bash
# Copy tactical_profile.ep to target system
scp tactical_profile.ep user@target:/opt/engage/
```

**4. Unpack on target system:**
```bash
python3 eptool.py -pwd secure123 -i tactical_profile.ep -o deployed_profile.json
```

### Advanced Usage

**Pack with custom identity override:**
```bash
python3 eptool.py -pwd mypassword -i base_profile.json -o custom_profile.ep -identity special_identity.json
```

**Unpack for inspection:**
```bash
python3 eptool.py -pwd mypassword -i encrypted_profile.ep -o readable_profile.json
cat readable_profile.json | jq .
```

## Use Cases

### Tactical Deployment
- **Secure distribution** of Engage configurations
- **Encrypted provisioning** for sensitive environments
- **Centralized management** of multiple device configurations

### Development and Testing
- **Version control** of encrypted provisioning profiles
- **Automated deployment** pipelines
- **Configuration management** for different environments

### Security Compliance
- **Encrypted storage** of sensitive configuration data
- **Secure transport** of provisioning profiles
- **Access control** through password protection

## Troubleshooting

### Common Issues

**"is not a valid input file"**
- Ensure input JSON contains `engageProvisioning` section
- Verify JSON syntax is correct
- Check file permissions

**"is not an Engage Provisioning Profile"**
- Ensure input file starts with `Engage Provisioning|` header
- Verify file is a valid .ep file
- Check if file was corrupted during transfer

**Decryption errors**
- Verify password is correct
- Ensure file wasn't modified after encryption
- Check for file corruption

**File not found errors**
- Verify all referenced files exist
- Check file paths are correct
- Ensure proper file permissions

### Debug Tips

**Validate JSON before packing:**
```bash
python3 -m json.tool profile.json
```

**Check file integrity:**
```bash
# Verify .ep file header
head -c 20 profile.ep
# Should show: "Engage Provisioning|"
```

**Test with simple profile:**
```json
{
  "engageProvisioning": {
    "identities": ["@file://simple_identity.json"]
  }
}
```

## Security Considerations

### Password Management
- Use strong, unique passwords for each profile
- Store passwords securely (password managers)
- Rotate passwords regularly
- Never hardcode passwords in scripts

### File Handling
- Verify file integrity after transfer
- Use secure transfer methods (SCP, SFTP)
- Validate decrypted content before deployment
- Keep backups of original JSON files

### Access Control
- Limit access to provisioning profiles
- Use appropriate file permissions
- Monitor access to sensitive configurations
- Implement audit logging where possible

## Integration

EPT integrates with other Rally Tactical Systems tools:
- **Engage Platform**: Direct deployment of .ep files
- **Configuration Management**: Automated provisioning workflows
- **Security Tools**: Integration with certificate management
- **Deployment Systems**: CI/CD pipeline integration

## License

Copyright (c) 2022 Rally Tactical Systems, Inc.

## Support

For questions, issues, or contributions, please refer to the main project documentation or contact the development team.

Secure provisioning made simple! 🔐
