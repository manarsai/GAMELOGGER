import { useState } from "react";
import { useNavigate } from "react-router-dom";

function Profile() {
    const navigate = useNavigate();

    const user = JSON.parse(localStorage.getItem("user"));

    const [username, setUsername] = useState(user.username);
    const [email, setEmail] = useState(user.email);
    const [password, setPassword] = useState("");
    const [message, setMessage] = useState("");

    async function saveDetails() {
        const response = await fetch(
            `http://localhost:8080/api/users/${user.id}`,
            {
                method: "PUT",
                headers: {
                    "Content-Type": "application/json"
                },
                body: JSON.stringify({ username, email })
            }
        );

        if (response.ok) {
            const updated = { ...user, username, email };
            localStorage.setItem("user", JSON.stringify(updated));
            setMessage("Account updated.");
        }
    }

    async function changePassword() {
        if (password.length < 6) {
            setMessage("Password must be at least 6 characters.");
            return;
        }

        const response = await fetch(
            `http://localhost:8080/api/users/${user.id}/password`,
            {
                method: "POST",
                headers: {
                    "Content-Type": "application/json"
                },
                body: JSON.stringify({ password })
            }
        );

        if (response.ok) {
            setPassword("");
            setMessage("Password changed.");
        }
    }

    async function deleteAccount() {
        if (!window.confirm("Delete your account permanently?")) return;

        const response = await fetch(
            `http://localhost:8080/api/users/${user.id}`,
            {
                method: "DELETE"
            }
        );

        if (response.ok) {
            localStorage.removeItem("user");
            navigate("/login");
        }
    }

    return (
        <div className="profile-page">

            <h1>Profile</h1>

            {message && <p>{message}</p>}

            <div className="profile-card">

                <label>Username</label>

                <input
                    value={username}
                    onChange={(e) => setUsername(e.target.value)}
                />

                <label>Email</label>

                <input
                    value={email}
                    onChange={(e) => setEmail(e.target.value)}
                />

                <button onClick={saveDetails}>
                    Save Changes
                </button>

                <hr />

                <label>New Password</label>

                <input
                    type="password"
                    value={password}
                    onChange={(e) => setPassword(e.target.value)}
                    placeholder="New password"
                />

                <button onClick={changePassword}>
                    Change Password
                </button>

                <hr />

                <div className="danger-zone">
                    <h3>Danger Zone</h3>
                    <p>Deleting your account will permanently remove your profile and all games in your library.</p>

                    <button
                        className="delete-button"
                        onClick={deleteAccount}
                    >
                        Delete Account
                    </button>
                </div>

            </div>

        </div>
    );
}

export default Profile;