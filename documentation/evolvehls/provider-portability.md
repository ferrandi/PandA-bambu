# Provider portability

PAF-01 defines a provider-neutral profile and diagnostic boundary. Profiles declare protocol surfaces and capability status without storing endpoints, tokens, hostnames, or vendor model identifiers. The example files use JSON syntax, which is valid YAML 1.2 and can be parsed safely with Python's standard library.

The proposed local work interfaces are `EVOLVEHLS_WORK_API_BASE_URL`, `EVOLVEHLS_WORK_API_TOKEN`, and `EVOLVEHLS_WORK_MODEL`. They are repository conventions, not claims about existing user or work environments. Local fixtures use separate `EVOLVEHLS_LOCAL_*` variables.

Codex custom providers require an OpenAI Responses-compatible surface. Claude Code commonly needs Anthropic Messages, while Cline may use OpenAI-compatible Chat Completions or another configured surface. Actual work-gateway compatibility is **unknown until explicitly probed**.

## Safety model

`doctor.py` only discovers executables, asks them for bounded version output, and parses known project-local configuration. It never modifies configuration and reports parse failures without file contents.

`probe_gateway.py` validates a profile, probes only repeated `--protocol` selections, and resolves endpoint, authentication, and model only at execution time. `--dry-run` does not read runtime values, execute token helpers, or access the network. Token helpers are argv arrays executed without a shell and with a timeout. Reports contain normalized status only; endpoint, headers, tokens, confidential model values, and response bodies are never emitted. HTTP error bodies and exception URLs are not printed.

The probe uses a synthetic prompt and reports authentication, protocol compatibility, observed streaming/tool/structured-output status, timeout observation and an explicit cancellation-not-probed status, and presence of usage metadata. Statuses distinguish success, unsupported protocol, authentication failure, transport failure, malformed response, and timeout. Exit codes are 0 for success/dry-run, 2 for protocol/capability failure, 3 for configuration failure, and 4 for transport/timeout failure.

Examples:

```bash
python3 tools/agentic/doctor.py --json
python3 tools/agentic/probe_gateway.py agentic/providers/work.example.yaml --protocol openai-responses --dry-run --json
```

A real probe must be intentionally configured through the declared environment variables. Local output belongs under ignored `agentic/probe-output/`.
