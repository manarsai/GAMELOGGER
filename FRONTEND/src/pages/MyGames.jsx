import { useEffect, useState } from "react";
import { Link } from "react-router-dom";


function MyGames() {
    const [games, setGames] = useState([]);
    const [editingId, setEditingId] = useState(null);

    const [title, setTitle] = useState("");
    const [platform, setPlatform] = useState("PC");
    const [status, setStatus] = useState("Want to Play");
    const [rating, setRating] = useState(0);
    const [hours, setHours] = useState(0);

    const [search, setSearch] = useState("");
    const [filterStatus, setFilterStatus] = useState("All");

    const [loading, setLoading] = useState(true);
    const [error, setError] = useState("");

    const user = JSON.parse(localStorage.getItem("user"));

    useEffect(() => {
        if (!user) {
            setError("Please log in first.");
            setLoading(false);
            return;
        }

        fetch(`http://localhost:8080/api/library?user_id=${user.id}`)
            .then((response) => {
                if (!response.ok) {
                    throw new Error(`Backend returned ${response.status}`);
                }
                return response.json();
            })
            .then((data) => setGames(data))
            .catch(() => setError("Could not load your games."))
            .finally(() => setLoading(false));
    }, [user?.id]);

    function handleEdit(game) {
        setError("");
        setEditingId(game.id);

        setTitle(game.title || "");
        setPlatform(game.platform || "PC");
        setStatus(game.status || "Want to Play");
        setRating(game.rating || 0);
        setHours(game.hours || 0);
    }

    async function handleSubmit(e) {
        e.preventDefault();

        if (!title.trim()) {
            setError("Game title is required.");
            return;
        }

        const currentGame = games.find((g) => g.id === editingId);

        if (!currentGame) {
            setError("Game could not be found.");
            return;
        }

        const updatedGame = {
            title: title.trim(),
            platform,
            status,
            rating: Number(rating),
            hours: Number(hours),
            thumbnail: currentGame.thumbnail || "",
            genre: currentGame.genre || "",
            publisher: currentGame.publisher || "",
            release_date: currentGame.release_date || "",
        };

        try {
            setError("");

            const response = await fetch(
                `http://localhost:8080/api/library/${editingId}`,
                {
                    method: "PUT",
                    headers: {
                        "Content-Type": "application/json",
                    },
                    body: JSON.stringify(updatedGame),
                }
            );

            if (!response.ok) {
                throw new Error();
            }

            setGames((currentGames) =>
                currentGames.map((game) =>
                    game.id === editingId
                        ? { ...game, ...updatedGame }
                        : game
                )
            );

            resetForm();
        } catch {
            setError("Could not update game.");
        }
    }

    async function handleDelete(id) {
        if (!window.confirm("Remove this game from My Games?")) {
            return;
        }

        try {
            const response = await fetch(
                `http://localhost:8080/api/library/${id}`,
                {
                    method: "DELETE",
                }
            );

            if (!response.ok) {
                throw new Error();
            }

            setGames((currentGames) =>
                currentGames.filter((game) => game.id !== id)
            );

            if (editingId === id) {
                resetForm();
            }
        } catch {
            setError("Could not remove game.");
        }
    }

    function resetForm() {
        setTitle("");
        setPlatform("PC");
        setStatus("Want to Play");
        setRating(0);
        setHours(0);
        setEditingId(null);
    }

    const filteredGames = games.filter((game) => {
        const matchesSearch = game.title
            ?.toLowerCase()
            .includes(search.toLowerCase());

        const matchesStatus =
            filterStatus === "All" || game.status === filterStatus;

        return matchesSearch && matchesStatus;
    });

    if (loading) {
        return (
            <div className="my-games-page">
                <h1>My Games</h1>
                <p>Loading your games...</p>
            </div>
        );
    }

    if (error && games.length === 0) {
        return (
            <div className="my-games-page">
                <h1>My Games</h1>

                <p className="error-message">{error}</p>

                <Link to="/games">
                    <button className="empty-button">
                        Browse Games
                    </button>
                </Link>
            </div>
        );
    }

    return (
        <div className="my-games-page">
            <h1>My Games</h1>

            <p>
                {games.length} game{games.length !== 1 ? "s" : ""} in your
                library.
            </p>

            {error && (
                <p className="error-message">{error}</p>
            )}

            <div className="controls">
                <input
                    type="text"
                    placeholder="Search my games..."
                    value={search}
                    onChange={(e) =>
                        setSearch(e.target.value)
                    }
                />

                <select
                    value={filterStatus}
                    onChange={(e) =>
                        setFilterStatus(e.target.value)
                    }
                >
                    <option value="All">All Games</option>
                    <option value="Want to Play">
                        Want to Play
                    </option>
                    <option value="Playing">
                        Playing
                    </option>
                    <option value="Completed">
                        Completed
                    </option>
                </select>
            </div>

            <p>
                Showing {filteredGames.length} of {games.length} games
            </p>

            {editingId !== null && (
                <form
                    className="edit-form"
                    onSubmit={handleSubmit}
                >
                    <h2>Edit Game</h2>

                    <label>Title</label>
                    <input
                        type="text"
                        value={title}
                        onChange={(e) =>
                            setTitle(e.target.value)
                        }
                    />

                    <label>Platform</label>
                    <select
                        value={platform}
                        onChange={(e) =>
                            setPlatform(e.target.value)
                        }
                    >
                        <option value="PC">PC</option>
                        <option value="PlayStation 5">
                            PlayStation 5
                        </option>
                        <option value="Xbox Series X">
                            Xbox Series X
                        </option>
                        <option value="Nintendo Switch">
                            Nintendo Switch
                        </option>
                    </select>

                    <label>Status</label>
                    <select
                        value={status}
                        onChange={(e) =>
                            setStatus(e.target.value)
                        }
                    >
                        <option value="Want to Play">
                            Want to Play
                        </option>
                        <option value="Playing">
                            Playing
                        </option>
                        <option value="Completed">
                            Completed
                        </option>
                    </select>

                    <label>Rating</label>
                    <select
                        value={rating}
                        onChange={(e) =>
                            setRating(e.target.value)
                        }
                    >
                        <option value="0">
                            Not Rated
                        </option>
                        <option value="1">
                            1 / 5
                        </option>
                        <option value="2">
                            2 / 5
                        </option>
                        <option value="3">
                            3 / 5
                        </option>
                        <option value="4">
                            4 / 5
                        </option>
                        <option value="5">
                            5 / 5
                        </option>
                    </select>

                    <label>Hours Played</label>
                    <input
                        type="number"
                        min="0"
                        step="0.5"
                        value={hours}
                        onChange={(e) =>
                            setHours(e.target.value)
                        }
                    />

                    <div className="button-group">
                        <button
                            type="submit"
                            className="btn-primary"
                        >
                            Save Changes
                        </button>

                        <button
                            type="button"
                            className="btn-secondary"
                            onClick={resetForm}
                        >
                            Cancel
                        </button>
                    </div>
                </form>
            )}

            {filteredGames.length === 0 ? (
                <div className="empty-state">
                    <h2>Your library is empty</h2>
                    <p>
                        Browse games and add some to your library.
                    </p>

                    <Link to="/games">
                        <button className="empty-button">
                            Browse Games
                        </button>
                    </Link>
                </div>
            ) : (
                <div className="games-grid">
                    {filteredGames.map((game) => (
                        <div
                            key={game.id}
                            className="game-card"
                        >
                            {game.thumbnail && (
                                <img
                                    src={game.thumbnail}
                                    alt={game.title}
                                />
                            )}

                            <div className="game-content">
                                <h2>
                                    <Link
                                        to={`/games/${game.id}`}
                                    >
                                        {game.title}
                                    </Link>
                                </h2>

                                <p>
                                    **Platform:** {game.platform}
                                </p>

                                <p>
                                    **Genre:** {game.genre}
                                </p>

                                <p>
                                    **Publisher:** {game.publisher}
                                </p>

                                <p>
                                    **Release:**{" "}
                                    {game.release_date}
                                </p>

                                <p>
                                    **Status:** {game.status}
                                </p>

                                <p>
                                    **Rating:**{" "}
                                    {game.rating === 0
                                        ? "Not Rated"
                                        : `${game.rating}/5`}
                                </p>

                                <p>
                                    **Hours Played:** {game.hours}
                                </p>

                                <div className="game-actions">
                                    <button
                                        className="btn-primary"
                                        onClick={() =>
                                            handleEdit(
                                                game
                                            )
                                        }
                                    >
                                        Edit
                                    </button>

                                    <button
                                        className="btn-danger"
                                        onClick={() =>
                                            handleDelete(
                                                game.id
                                            )
                                        }
                                    >
                                        Remove
                                    </button>
                                </div>
                            </div>
                        </div>
                    ))}
                </div>
            )}
        </div>
    );
}

export default MyGames;