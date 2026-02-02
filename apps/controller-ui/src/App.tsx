import { useEffect, useState } from 'react';
import { useQuery } from '@tanstack/react-query';

import { RulerPicker } from './ruler-picker';

const API_URL = 'http://localhost:8080';

interface State {
  gain_db: number;
  reverb_delay_ms: number;
  reverb_feedback: number;
  reverb_wet: number;
  reverb_dry: number;
  dc_blocker_cutoff_hz: number;
}

function App() {
  const {
    data,
    isError,
    isPending,
    error
  } = useQuery<State>({
    queryKey: ['state'],
    queryFn: async () => {
      const response = await fetch(`${API_URL}/state`);
      if (!response.ok) {
        throw new Error(`Request failed: ${response.status}`);
      }
      return (await response.json()) as State;
    }
  });

  // Set initial values for RulerPickers based on their intended ranges and defaults
  const [gainValue, setGainValue] = useState(0);
  const [delayValue, setDelayValue] = useState(0);
  const [reverbFeedback, setReverbFeedback] = useState(0);   // In percent, default 0.25*100
  const [reverbWet, setReverbWet] = useState(0);             // In percent, default 0.55*100
  const [reverbDry, setReverbDry] = useState(0);             // In percent, default 0.8*100
  const [dcBlockerCutoffHz, setDcBlockerCutoffHz] = useState(0);

  const [status, setStatus] = useState<string | null>(null);

  // Populate pickers with backend state when fetched
  useEffect(() => {
    if (data) {
      setGainValue(data.gain_db);
      setDelayValue(data.reverb_delay_ms);
      setReverbFeedback(Math.round(data.reverb_feedback * 100));
      setReverbWet(Math.round(data.reverb_wet * 100));
      setReverbDry(Math.round(data.reverb_dry * 100));
      setDcBlockerCutoffHz(data.dc_blocker_cutoff_hz);
    }
  }, [data]);

  // Submit updated state to backend
  const submit = async () => {
    setStatus(null);
    try {
      const d = {
        gain_db: gainValue,
        reverb_delay_ms: delayValue,
        reverb_feedback: reverbFeedback / 100,
        reverb_wet: reverbWet / 100,
        reverb_dry: reverbDry / 100,
        dc_blocker_cutoff_hz: dcBlockerCutoffHz,
      };

      const params = new URLSearchParams();
      for (const [key, value] of Object.entries(d)) {
        params.set(key, String(value));
      }
      const response = await fetch(`${API_URL}/state?${params.toString()}`, {
        method: 'PATCH'
      });
      if (!response.ok) {
        throw new Error(`Update failed: ${response.status}`);
      }
      setStatus('Updated.');
    } catch (err) {
      setStatus((err as Error).message);
    }
  };

  // If you want to display loading or error state (fix for UX)
  if (isPending) {
    return (
      <div className="min-h-screen bg-black flex items-center justify-center" style={{ height: "100vh" }}>
        <span className="text-zinc-300">Loading...</span>
      </div>
    );
  }

  if (isError) {
    return (
      <div className="min-h-screen bg-black flex items-center justify-center" style={{ height: "100vh" }}>
        <span className="text-red-400">Error: {(error as Error).message}</span>
      </div>
    );
  }

  return (
    <div className="min-h-screen bg-black" style={{ height: "100vh" }}>
      <div className="w-full h-full flex gap-8">
        <RulerPicker
          min={0}
          max={60}
          step={5}
          suffix="db"
          value={gainValue}
          onChange={setGainValue}
        />
        <RulerPicker
          min={0}
          max={2000}
          step={10}
          suffix="ms"
          value={delayValue}
          onChange={setDelayValue}
        />
        <RulerPicker
          min={0}
          max={100}
          step={5}
          suffix="%"
          value={reverbFeedback}
          onChange={setReverbFeedback}
        />
        <RulerPicker
          min={0}
          max={100}
          step={5}
          suffix="wet %"
          value={reverbWet}
          onChange={setReverbWet}
        />
        <RulerPicker
          min={0}
          max={100}
          step={5}
          suffix="dry %"
          value={reverbDry}
          onChange={setReverbDry}
        />
        <RulerPicker
          min={0}
          max={100}
          step={5}
          suffix="hz"
          value={dcBlockerCutoffHz}
          onChange={setDcBlockerCutoffHz}
        />
      </div>
      <div className="pt-8 flex gap-2 items-center">
        <button
          className="bg-zinc-800 rounded px-3 py-2 text-white"
          onClick={submit}
        >
          Spara
        </button>
        {status && <span className="text-zinc-300">{status}</span>}
      </div>
    </div>
  );
}

export default App;
