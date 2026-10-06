import { useEffect, useState } from "react";
import { Link } from "react-router-dom";
// page doesnt appear in frontend 
function Home() {
    const [games, setGames] = useState([]);
    const [backendStatus, setBackendStatus] =
        useState("Checking...");

    useEffect(() => {
        // load My Games
        const savedGames =
            localStorage.getItem("gamelog-games");

        if (savedGames) {
            setGames(JSON.parse(savedGames));
        }

        // check Drogon backend
        fetch("http://localhost:8080/api/health")
            .then((response) => {
                if (!response.ok) {
                    throw new Error("Backend offline");
                }

                return response.json();
            })
            .then((data) => {
                console.log(
                    "Backend response:",
                    data
                );

                setBackendStatus("Online");
            })
            .catch((error) => {
                console.error(
                    "Backend error:",
                    error
                );

                setBackendStatus("Offline");
            });
    }, []);

    const totalGames = games.length;

    const playingGames = games.filter(
        (game) => game.status === "Playing"
    ).length;

    const completedGames = games.filter(
        (game) => game.status === "Completed"
    ).length;

    const wantToPlayGames = games.filter(
        (game) => game.status === "Want to Play"
    ).length;

    const totalHours = games.reduce(
        (total, game) =>
            total + Number(game.hours || 0),
        0
    );

    const ratedGames = games.filter(
        (game) => Number(game.rating) > 0
    );

    const averageRating =
        ratedGames.length > 0
            ? (
                ratedGames.reduce(
                    (total, game) =>
                        total +
                        Number(game.rating),
                    0
                ) / ratedGames.length
            ).toFixed(1)
            : "N/A";

    return (
        <div>
 


            <div className="card">
                <h2>Your Library</h2>

                {totalGames === 0 ? (
                    <>
                        <p>
                            You haven't added
                            any games yet.
                        </p>

                        <Link
                            to="/games"
                            className="button"
                        >
                            Browse Games
                        </Link>
                    </>
                ) : (
                    <>
                        <p>
                            You have{" "}
                            <strong>
                                {totalGames}
                            </strong>{" "}
                            games in your
                            library.
                        </p>

                        <Link
                            to="/my-games"
                            className="button"
                        >
                            View My Games
                        </Link>
                    </>
                )}
            </div>
        </div>
    );
}

export default Home;