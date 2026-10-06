import { useEffect, useState } from "react";
import { Link, useNavigate } from "react-router-dom";

function Games() {
    const [games, setGames] = useState([]);

    const navigate = useNavigate();

    const [title, setTitle] = useState("");
    const [platform, setPlatform] = useState("PC");
    const [status, setStatus] = useState("Want to Play");

    const [editingId, setEditingId] = useState(null);

    const [search, setSearch] = useState("");
    const [filterStatus, setFilterStatus] = useState("All");

    const [loading, setLoading] = useState(false);
    const [apiError, setApiError] = useState("");

    const [addingId, setAddingId] = useState(null);

    const user = JSON.parse(
        localStorage.getItem("user")
    );

    // logout
    function handleLogout() {
        localStorage.removeItem("user");
        navigate("/login");
    }

    // load games from free to game api 

    useEffect(() => {
        setLoading(true);
        setApiError("");

        fetch("http://localhost:8080/api/games")
            .then((response) => {
                if (!response.ok) {
                    throw new Error(
                        `Backend returned ${response.status}`
                    );
                }

                return response.json();
            })
            .then((data) => {
                console.log(
                    "Games from backend:",
                    data
                );

                const apiGames = data.map((game) => ({
                    id: game.id,
                    title: game.title,
                    platform:
                        game.platform || "PC",
                    status: "Want to Play",
                    rating: 0,
                    hours: 0,
                    thumbnail:
                        game.thumbnail || "",
                    genre:
                        game.genre || "",
                    publisher:
                        game.publisher || "",
                    release_date:
                        game.release_date || "",
                }));

                setGames(apiGames);
            })
            .catch((error) => {
                console.error(
                    "Games API error:",
                    error
                );

                setApiError(
                    "Could not load games from the backend."
                );
            })
            .finally(() => {
                setLoading(false);
            });
    }, []);

    // add game to user library

    async function handleAddToLibrary(game) {
        if (!user) {
            alert("Please log in first.");
            navigate("/login");
            return;
        }

        try {
            setAddingId(game.id);

            const response = await fetch(
                "http://localhost:8080/api/library",
                {
                    method: "POST",

                    headers: {
                        "Content-Type": "application/json",
                    },

                    body: JSON.stringify({
                        id: game.id,
                        user_id: user.id,
                        title: game.title,
                        platform:
                            game.platform,
                        status: "Want to Play",
                        rating: 0,
                        hours: 0,
                        thumbnail:
                            game.thumbnail || "",
                        genre:
                            game.genre || "",
                        publisher:
                            game.publisher || "",
                        release_date:
                            game.release_date || "",
                    }),
                }
            );

            if (!response.ok) {
                throw new Error(
                    `Backend returned ${response.status}`
                );
            }

            const data =
                await response.json();

            console.log(
                "Added to My Games:",
                data
            );

            alert(
                `${game.title} added to My Games!`
            );
        } catch (error) {
            console.error(
                "Add to library error:",
                error
            );

            alert(
                "Could not add game to My Games."
            );
        } finally {
            setAddingId(null);
        }
    }

    // submit

    async function handleSubmit(event) {
        event.preventDefault();

        if (!user) {
            alert("Please log in first.");
            navigate("/login");
            return;
        }

        if (!title.trim()) {
            return;
        }

        const existingGame =
            editingId !== null
                ? games.find(
                    (game) =>
                        game.id === editingId
                )
                : null;

        const gameData = {
            id:
                editingId !== null
                    ? editingId
                    : Date.now(),

            user_id: user.id,

            title: title.trim(),

            platform,

            status,

            rating:
                existingGame?.rating ?? 0,

            hours:
                existingGame?.hours ?? 0,

            thumbnail:
                existingGame?.thumbnail ?? "",

            genre:
                existingGame?.genre ?? "",

            publisher:
                existingGame?.publisher ?? "",

            release_date:
                existingGame?.release_date ?? "",
        };

        try {
            //update exisitng games

            if (editingId !== null) {
                const response = await fetch(
                    `http://localhost:8080/api/library/${editingId}`,
                    {
                        method: "PUT",

                        headers: {
                            "Content-Type":
                                "application/json",
                        },

                        body: JSON.stringify(
                            gameData
                        ),
                    }
                );

                if (!response.ok) {
                    throw new Error(
                        `Backend returned ${response.status}`
                    );
                }

                setGames(
                    (currentGames) =>
                        currentGames.map(
                            (game) =>
                                game.id ===
                                    editingId
                                    ? {
                                        ...game,
                                        ...gameData,
                                    }
                                    : game
                        )
                );

                resetForm();

                console.log(
                    "Game updated successfully"
                );

                return;
            }

            // add new game
            const response = await fetch(
                "http://localhost:8080/api/library",
                {
                    method: "POST",

                    headers: {
                        "Content-Type":
                            "application/json",
                    },

                    body: JSON.stringify(
                        gameData
                    ),
                }
            );

            if (!response.ok) {
                throw new Error(
                    `Backend returned ${response.status}`
                );
            }

            setGames(
                (currentGames) => [
                    ...currentGames,
                    gameData,
                ]
            );

            resetForm();

            console.log(
                "Game added successfully"
            );
        } catch (error) {
            console.error(
                "Save game error:",
                error
            );

            setApiError(
                "Failed to save game."
            );
        }
    }

    // edit

    function handleEdit(game) {
        setEditingId(game.id);
        setTitle(game.title);
        setPlatform(game.platform);
        setStatus(game.status);
    }

    // reset form 

    function resetForm() {
        setTitle("");
        setPlatform("PC");
        setStatus("Want to Play");
        setEditingId(null);
    }

    //filter

    const filteredGames = games.filter(
        (game) => {
            const matchesSearch =
                game.title
                    .toLowerCase()
                    .includes(
                        search.toLowerCase()
                    );

            const matchesStatus =
                filterStatus === "All" ||
                game.status ===
                filterStatus;

            return (
                matchesSearch &&
                matchesStatus
            );
        }
    );

    // ui

    return (
        <div className="games-page">

            <div className="page-header">
                <h1> Games</h1>
            </div>

            {user && (
                <div className="welcome-banner">
                    <p>
                        Welcome, <strong>{user.username}</strong>!
                    </p>

                    <button
                        className="btn-danger"
                        onClick={handleLogout}
                    >
                        Logout
                    </button>
                </div>
            )}

            {loading && (
                <p className="loading-message">
                    Loading games...
                </p>
            )}

            {apiError && (
                <p className="error-message">
                    {apiError}
                </p>
            )}

            {/* SEARCH & FILTER */}

            <div className="games-controls">

                <input
                    type="text"
                    placeholder="Search games..."
                    value={search}
                    onChange={(event) =>
                        setSearch(event.target.value)
                    }
                />

                {/*<select*/}
                {/*    value={filterStatus}*/}
                {/*    onChange={(event) =>*/}
                {/*        setFilterStatus(event.target.value)*/}
                {/*    }*/}
                {/*>*/}
                {/*    <option value="All">*/}
                {/*        All Games*/}
                {/*    </option>*/}

                {/*    <option value="Playing">*/}
                {/*        Playing*/}
                {/*    </option>*/}

                {/*    <option value="Completed">*/}
                {/*        Completed*/}
                {/*    </option>*/}

                {/*    <option value="Want to Play">*/}
                {/*        Want to Play*/}
                {/*    </option>*/}
                {/*</select>*/}

            </div>

            <p>
                Showing{" "}
                <strong>{filteredGames.length}</strong> of{" "}
                <strong>{games.length}</strong> games
            </p>

            {/* GAME LIST */}

            {filteredGames.length === 0 ? (

                <div className="empty-message">
                    <h2>No games found</h2>
                    <p>
                        Try changing your search or filter.
                    </p>
                </div>

            ) : (

                <div className="games-grid">

                    {filteredGames.map((game) => (

                        <div
                            className="game-card"
                            key={game.id}
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
                                    <strong>Platform:</strong>{" "}
                                    {game.platform}
                                </p>

                                <p>
                                    <strong>Genre:</strong>{" "}
                                    {game.genre || "Unknown"}
                                </p>

                                <p>
                                    <strong>Publisher:</strong>{" "}
                                    {game.publisher || "Unknown"}
                                </p>

                                <div className="game-actions">

                                    <button
                                        className="btn-primary"
                                        onClick={() =>
                                            handleAddToLibrary(game)
                                        }
                                        disabled={
                                            addingId === game.id
                                        }
                                    >
                                        {addingId === game.id
                                            ? "Adding..."
                                            : "Add to My Games"}
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

export default Games;