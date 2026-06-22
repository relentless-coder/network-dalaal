#!/usr/bin/env bash
# Integration tests for the C reverse proxy / load balancer.

set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PROXY_BIN="$PROJECT_ROOT/server"
BACKEND_PY="$PROJECT_ROOT/tests/backend.py"

PROXY_PID=""
BACKEND_PIDS=()

cleanup() {
    echo "[cleanup] shutting down test servers..."
    if [[ -n "$PROXY_PID" ]]; then
        kill "$PROXY_PID" 2>/dev/null || true
        wait "$PROXY_PID" 2>/dev/null || true
    fi
    for pid in "${BACKEND_PIDS[@]}"; do
        kill "$pid" 2>/dev/null || true
        wait "$pid" 2>/dev/null || true
    done
}
trap cleanup EXIT

fail() {
    echo "FAIL: $1" >&2
    exit 1
}

# NOTE: build the server binary before running this test, e.g.:
#   cd "$PROJECT_ROOT" && gcc -o server src/main.c
cd "$PROJECT_ROOT"
if [[ ! -x "$PROXY_BIN" ]]; then
    fail "server binary not found at $PROXY_BIN — build it first"
fi

echo "[setup] starting backends on 9001, 9002, 9003..."
for port in 9001 9002 9003; do
    python3 "$BACKEND_PY" "$port" &
    BACKEND_PIDS+=("$!")
done

# Wait for backends to be ready
for port in 9001 9002 9003; do
    for _ in {1..20}; do
        if curl -s "http://127.0.0.1:$port/" >/dev/null 2>&1; then
            break
        fi
        sleep 0.1
    done
done

echo "[setup] starting proxy on 8080..."
"$PROXY_BIN" &
PROXY_PID="$!"

# Wait for proxy to be ready
for _ in {1..30}; do
    if curl -s "http://127.0.0.1:8080/" >/dev/null 2>&1; then
        break
    fi
    sleep 0.1
done

echo "[test] round-robin across three backends..."
seen_9001=0
seen_9002=0
seen_9003=0
for _ in {1..6}; do
    body=$(curl -s "http://127.0.0.1:8080/")
    case "$body" in
        "backend 9001") seen_9001=1 ;;
        "backend 9002") seen_9002=1 ;;
        "backend 9003") seen_9003=1 ;;
        *) fail "unexpected response: $body" ;;
    esac
done
(( seen_9001 == 1 )) || fail "never hit backend 9001"
(( seen_9002 == 1 )) || fail "never hit backend 9002"
(( seen_9003 == 1 )) || fail "never hit backend 9003"
echo "[test] all three backends were reached"

echo "[test] dead backend is skipped..."
kill "${BACKEND_PIDS[2]}"  # stop backend 9003
wait "${BACKEND_PIDS[2]}" 2>/dev/null || true
BACKEND_PIDS=("${BACKEND_PIDS[@]:0:2}")

sleep 0.5
seen_dead=0
for _ in {1..6}; do
    body=$(curl -s "http://127.0.0.1:8080/")
    if [[ "$body" == "backend 9003" ]]; then
        seen_dead=1
    fi
done
(( seen_dead == 0 )) || fail "proxy still routed to dead backend 9003"
echo "[test] dead backend was not reached"

echo "[test] all backends dead returns 502..."
kill "${BACKEND_PIDS[@]}"
wait "${BACKEND_PIDS[0]}" 2>/dev/null || true
wait "${BACKEND_PIDS[1]}" 2>/dev/null || true
BACKEND_PIDS=()

sleep 0.5
status=$(curl -s -o /dev/null -w "%{http_code}" "http://127.0.0.1:8080/" || true)
[[ "$status" == "502" ]] || fail "expected 502 when all backends dead, got: $status"
echo "[test] got 502 as expected"

echo ""
echo "PASS: all integration tests passed"
