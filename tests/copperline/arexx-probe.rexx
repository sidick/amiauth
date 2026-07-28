/* arexx-probe.rexx — drives AmiAuthGUI's ARexx port (#46) for the on-target
 * smoke test (tests/gui/arexx-onhw.sh). Run via `rx` under a real resident
 * RexxMast so RC/RESULT populate exactly as a real user's script would see.
 *
 * OPTIONS RESULTS is required — without it ARexx never sets RXFF_RESULT on
 * the outgoing RexxMsg, so the host never gets asked for a RESULT string at
 * all (confirmed empirically: rm_Action came back plain RXCOMM, no result
 * bit, until this was added). Separately, ARexx 'drops' (uninitializes)
 * RESULT when a host command doesn't supply one; referencing a dropped
 * variable yields its own name ("RESULT") per REXX semantics. SYMBOL()
 * below turns that into a clean empty string so every line has a
 * predictable "RESULT=<text or nothing>" shape. */
OPTIONS RESULTS
ADDRESS AMIAUTH.1
CALL PROBE 'STATUS', 'STATUS'
CALL PROBE 'LIST', 'LIST'
CALL PROBE 'GETCODE smoke', 'GETCODE'
CALL PROBE 'TIMELEFT smoke', 'TIMELEFT'
CALL PROBE 'GETCODE nosuchaccount', 'NOTFOUND'
CALL PROBE 'BOGUSCOMMAND', 'UNKNOWN'
CALL PROBE 'QUIT', 'QUIT'
EXIT

PROBE: PROCEDURE
  cmd = ARG(1)
  tag = ARG(2)
  cmd
  if symbol('RESULT') = 'VAR' then r = RESULT
  else r = ''
  SAY tag' RC='RC' RESULT='r
  RETURN
