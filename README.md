# Test tool

## Secrets
Usernames and passwords are read from environment variables (`${VAR}` in `tests.yaml`).
Copy `.env.example` to `.env` and fill in the values. `.env` must never be committed.

## Running the Docker test image
```bash
docker build -t domibus-tester .
docker run --rm --env-file .env --network domibus-lab domibus-tester run <test_name|all>
```

Debugging: add `-e DOMIBUS_DEBUG=1` to print the SOAP header that is sent.
