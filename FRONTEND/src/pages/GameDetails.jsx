import { useEffect, useState } from "react";
import { Link, useParams } from "react-router-dom";

function GameDetails() {
    const { id } = useParams();

    const [game, setGame] = useState(null);
    const [loading, setLoading] = useState(true);
    const [error, setError] = useState("");

    useEffect(() => {
        fetch(`http://localhost:8080/api/games/${id}`)
            .then((response) => {
                if (!response.ok) {
                    throw new Error(
                        `Backend returned ${response.status}`
                    );
                }

                return response.json();
            })
            .then((data) => {
                setGame(data);
            })
            .catch((error) => {
                console.error("Game details error:", error);
                setError("Could not load game details.");
            })
            .finally(() => {
                setLoading(false);
            });
    }, [id]);

    if (loading) {
        return <p>Loading game...</p>;
    }

    if (error) {
        return (
            <div>
                <h1>Error</h1>
                <p>{error}</p>

                <Link to="/games">
                    Back to My Games
                </Link>
            </div>
        );
    }

    if (!game) {
        return (
            <div>
                <h1>Game not found</h1>

                <Link to="/games">
                    Back to My Games
                </Link>
            </div>
        );
    }

    return (
        <div>
            <Link to="/games">
                ? Back to My Games
            </Link>

            <h1>{game.title}</h1>

            {game.thumbnail && (
                <img
                    src={game.thumbnail}
                    alt={game.title}
                    width="400"
                />
            )}

            <p>
                <strong>Genre:</strong>{" "}
                {game.genre || "Unknown"}
            </p>

            <p>
                <strong>Publisher:</strong>{" "}
                {game.publisher || "Unknown"}
            </p>

            <p>
                <strong>Release date:</strong>{" "}
                {game.release_date || "Unknown"}
            </p>

            <p>
                <strong>Platform:</strong>{" "}
                {game.platform || "Unknown"}
            </p>

            <p>
                <strong>Status:</strong>{" "}
                {game.status || "Want to Play"}
            </p>

            <p>
                <strong>Rating:</strong>{" "}
                {game.rating ?? 0}/5
            </p>
        </div>
    );
}

export default GameDetails;