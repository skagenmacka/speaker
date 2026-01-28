import { useQuery } from '@tanstack/react-query';

const API_URL = 'http://localhost:8080';

interface State {
  gain_db: number;

  reverb_delay_ms: number;
  reverb_feedback: number;
  reverb_wet: number;
  reverb_dry: number;
};

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
    },
  });

  return (
    <>
      <div>
        {isPending && <p>Loading...</p>}
        {isError && <p>Error: {(error as Error).message}</p>}
        {data && <pre>{JSON.stringify(data, null, 2)}</pre>}
      </div>
    </>
  )
}

export default App;
