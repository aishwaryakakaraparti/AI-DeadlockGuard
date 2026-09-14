import urllib.request, json, sys
try:
    with urllib.request.urlopen('http://localhost:5050/api/state', timeout=5) as r:
        data = json.loads(r.read())
    print('API /api/state: OK')
    print('monitor_status:', data.get('monitor_status'))
    print('risk_score:',     data.get('risk_score'))
    print('features:',       data.get('features'))
    print('log_lines count:',len(data.get('log_lines', [])))
except Exception as e:
    print('ERROR:', e)
    sys.exit(1)
