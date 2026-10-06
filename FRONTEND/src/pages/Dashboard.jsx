import { useEffect, useState } from "react";

function Dashboard() {
    const [stats, setStats] = useState(null);
    const [loading, setLoading] = useState(true);
    const [error, setError] = useState("");

    const user = JSON.parse(localStorage.getItem("user"));

    useEffect(() => {
        if (!user) {
            setError("Please log in.");
            setLoading(false);
            return;
        }

        fetch(`http://localhost:8080/api/dashboard?user_id=${user.id}`)
            .then((response) => {
                if (!response.ok) {
                    throw new Error("Failed to load dashboard");
                }
                return response.json();
            })
            .then((data) => setStats(data))
            .catch(() => setError("Could not load dashboard."))
            .finally(() => setLoading(false));
    }, []);

    if (loading) return <p>Loading dashboard...</p>;
    if (error) return <p>{error}</p>;

    return (
        <div className="dashboard">
            <h1>Dashboard</h1>
            <p>Welcome back, {user.username}.</p>

            <div className="stats-grid">
                <div className="card">
                    <h2>{stats.total}</h2>
                    <p>Total Games</p>
                </div>

                <div className="card">
                    <h2>{stats.playing}</h2>
                    <p>Playing</p>
                </div>

                <div className="card">
                    <h2>{stats.completed}</h2>
                    <p>Completed</p>
                </div>

                <div className="card">
                    <h2>{stats.want_to_play}</h2>
                    <p>Want to Play</p>
                </div>

                <div className="card">
                    <h2>{stats.hours}</h2>
                    <p>Total Hours</p>
                </div>
            </div>
        </div>
    );
}

export default Dashboard;