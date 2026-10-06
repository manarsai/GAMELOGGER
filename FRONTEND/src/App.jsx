import { useEffect, useState } from "react";
import {
    BrowserRouter,
    Routes,
    Route,
    Link,
    useNavigate,
} from "react-router-dom";

import Register from "./pages/Register";
import Login from "./pages/Login";
//import Home from "./pages/Home";
import Games from "./pages/Games";
import GameDetails from "./pages/GameDetails";
import MyGames from "./pages/MyGames";
import Profile from "./pages/Profile";
import Dashboard from "./pages/Dashboard";

function Navbar({ user, setUser }) {
    const navigate = useNavigate();

    function handleLogout() {
        localStorage.removeItem("user");
        setUser(null);
        navigate("/login");
    }

    return (
        <nav>
            <Link to="#">GameLogger</Link>

            <Link to="/games">Browse Games</Link>

            {user && (
                <>
                    <Link to="/dashboard">Dashboard</Link>
                    <Link to="/my-games">My Games</Link>
                    <Link to="/profile">Profile</Link>

                    <button
                        className="logout-btn"
                        onClick={handleLogout}
                    >
                        Logout
                    </button>
                </>
            )}

            {!user && (
                <>
                    <Link to="/register">Register</Link>
                    <Link to="/login">Login</Link>
                </>
            )}
        </nav>
    );
}

function AppContent() {
    const [user, setUser] = useState(
        JSON.parse(localStorage.getItem("user"))
    );

    useEffect(() => {
        fetch("http://localhost:8080/api/health")
            .then((response) => response.json())
            .then((data) =>
                console.log("Backend response:", data)
            )
            .catch((error) =>
                console.error("Backend error:", error)
            );
    }, []);

    return (
        <>
            <Navbar user={user} setUser={setUser} />

            <main className="page">
                <Routes>
                    <Route path="/" element={<Games />} />

                    <Route path="/games" element={<Games />} />

                    <Route
                        path="/games/:id"
                        element={<GameDetails />}
                    />

                    <Route
                        path="/my-games"
                        element={<MyGames />}
                    />

                    <Route
                        path="/profile"
                        element={<Profile />}
                    />

                    <Route
                        path="/register"
                        element={<Register />}
                    />

                    <Route
                        path="/login"
                        element={
                            <Login setUser={setUser} />
                        }
                    />

                    <Route
                        path="/dashboard"
                        element={<Dashboard />}
                    />
                </Routes>
            </main>
        </>
    );
}

function App() {
    return (
        <BrowserRouter>
            <AppContent />
        </BrowserRouter>
    );
}

export default App;