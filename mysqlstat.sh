#!/bin/bash
# mysqlstat — MongoDB mongostat-style real-time monitor for MySQL
# Usage: ./mysqlstat.sh [host] [user] [pass] [interval_sec]

HOST="${1:-192.168.1.13}"
USER="${2:-root}"
PASS="${3:-}"
INTERVAL="${4:-1}"

# Hide password warning
export MYSQL_PWD="$PASS"

SQL="SELECT
  ROUND(ss.value - COALESCE(sv.value, ss.value)) AS 'Com_insert',
  ROUND(sq.value - COALESCE(qv.value, sq.value)) AS 'Com_select',
  ROUND(su.value - COALESCE(uv.value, su.value)) AS 'Com_update',
  ROUND(sd.value - COALESCE(dv.value, sd.value)) AS 'Com_delete',
  ROUND(sr.value - COALESCE(rv.value, sr.value)) AS 'Rows_read',
  ROUND(srw.value - COALESCE(rwv.value, srw.value)) AS 'Rows_written',
  ROUND(sc.value - COALESCE(cv.value, sc.value)) AS 'Connections',
  ROUND(st.value - COALESCE(tv.value, st.value)) AS 'Threads_running',
  ROUND(sq2.value - COALESCE(q2v.value, sq2.value)) AS 'Questions',
  ROUND(ssw.value - COALESCE(swv.value, ssw.value)) AS 'Slow_queries',
  ROUND(srb.value - COALESCE(rbv.value, srb.value)) AS 'Bytes_received',
  ROUND(sst.value - COALESCE(stv.value, sst.value)) AS 'Bytes_sent'
FROM (
  SELECT variable_name, variable_value AS value
  FROM information_schema.global_status
  WHERE variable_name IN (
    'Com_insert','Com_select','Com_update','Com_delete',
    'Innodb_rows_read','Innodb_rows_inserted','Innodb_rows_updated',
    'Threads_connected','Threads_running',
    'Questions','Slow_queries',
    'Bytes_received','Bytes_sent',
    'Uptime'
  )
) ss
LEFT JOIN (
  SELECT variable_name, variable_value AS value
  FROM information_schema.global_status
  WHERE variable_name IN (
    'Com_insert','Com_select','Com_update','Com_delete',
    'Innodb_rows_read','Innodb_rows_inserted','Innodb_rows_updated',
    'Threads_connected','Threads_running',
    'Questions','Slow_queries',
    'Bytes_received','Bytes_sent'
  )
) sv USING (variable_name)
CROSS JOIN (
  SELECT 1 AS dummy
) dummy
-- join each variable separately
"

# Simpler approach: just poll SHOW GLOBAL STATUS and diff
echo "mysqlstat — polling ${HOST}:3306 every ${INTERVAL}s"
echo ""
printf "%-10s %8s %8s %8s %8s %10s %8s %8s %10s\n" \
  "Time" "Insert" "Select" "Update" "Delete" "Running" "QPS" "Slow" "BytesRecv"

mysqladmin -h "$HOST" -u "$USER" extended-status -r -i "$INTERVAL" 2>/dev/null | \
  awk -v interval="$INTERVAL" '
  /Com_insert/   { ins = $4 - prev_ins;  prev_ins = $4 }
  /Com_select/   { sel = $4 - prev_sel;  prev_sel = $4 }
  /Com_update/   { upd = $4 - prev_upd;  prev_upd = $4 }
  /Com_delete/   { del = $4 - prev_del;  prev_del = $4 }
  /Threads_running/ { run = $4 }
  /Questions/    { qps = int(($4 - prev_q) / interval); prev_q = $4 }
  /Slow_queries/ { slow = $4 - prev_slow; prev_slow = $4 }
  /Bytes_received/ { brecv = int(($4 - prev_brecv) / interval); prev_brecv = $4 }
  {
    if (ins+0 > 0 || sel+0 > 0 || upd+0 > 0) {
      cmd = "date +%H:%M:%S"
      cmd | getline ts
      close(cmd)
      printf "%-10s %8d %8d %8d %8d %10s %8d %8d %10d\n", ts, ins, sel, upd, del, run, qps, slow, brecv
      ins=0; sel=0; upd=0; del=0; run=""; qps=0; slow=0; brecv=0
    }
  }'
