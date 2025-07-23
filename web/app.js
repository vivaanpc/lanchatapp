// LAN Chat Application JavaScript
class LanChat {
    constructor() {
        this.apiUrl = '';  // Base URL (empty for relative paths)
        this.username = localStorage.getItem('lanChatUsername') || '';
        this.theme = localStorage.getItem('lanChatTheme') || 'light';
        this.lastMessageCount = 0;
        this.pollInterval = null;
        this.peerPollInterval = null;
        
        this.initializeElements();
        this.initializeEventListeners();
        this.applyTheme();
        this.loadUsername();
        this.startPolling();
    }

    initializeElements() {
        // Main elements
        this.messagesContainer = document.getElementById('messages-container');
        this.messageInput = document.getElementById('message-input');
        this.sendBtn = document.getElementById('send-btn');
        this.statusText = document.getElementById('status-text');
        this.peerCount = document.getElementById('peer-count');
        
        // Modal elements
        this.settingsModal = document.getElementById('settings-modal');
        this.peersModal = document.getElementById('peers-modal');
        this.usernameInput = document.getElementById('username-input');
        this.themeSelect = document.getElementById('theme-select');
        this.peersList = document.getElementById('peers-list');
    }

    initializeEventListeners() {
        // Send message
        this.sendBtn.addEventListener('click', () => this.sendMessage());
        this.messageInput.addEventListener('keypress', (e) => {
            if (e.key === 'Enter' && !e.shiftKey) {
                e.preventDefault();
                this.sendMessage();
            }
        });

        // Settings modal
        document.getElementById('settings-btn').addEventListener('click', () => this.showSettings());
        document.getElementById('close-settings').addEventListener('click', () => this.hideSettings());
        document.getElementById('save-username').addEventListener('click', () => this.saveUsername());
        this.themeSelect.addEventListener('change', () => this.saveTheme());
        document.getElementById('clear-messages').addEventListener('click', () => this.clearMessages());

        // Peers modal
        document.getElementById('peers-btn').addEventListener('click', () => this.showPeers());
        document.getElementById('close-peers').addEventListener('click', () => this.hidePeers());

        // Close modals on background click
        window.addEventListener('click', (e) => {
            if (e.target.classList.contains('modal')) {
                e.target.classList.remove('show');
            }
        });

        // Username input enter key
        this.usernameInput.addEventListener('keypress', (e) => {
            if (e.key === 'Enter') {
                this.saveUsername();
            }
        });
    }

    loadUsername() {
        if (this.username) {
            this.usernameInput.value = this.username;
        } else {
            // Prompt for username on first use
            setTimeout(() => this.showSettings(), 500);
        }
    }

    applyTheme() {
        document.body.className = this.theme === 'dark' ? 'dark-theme' : '';
        this.themeSelect.value = this.theme;
    }

    saveUsername() {
        const newUsername = this.usernameInput.value.trim();
        if (newUsername && newUsername.length <= 50) {
            this.username = newUsername;
            localStorage.setItem('lanChatUsername', this.username);
            this.setStatus('Username saved');
            this.hideSettings();
        } else {
            alert('Please enter a valid username (1-50 characters)');
        }
    }

    saveTheme() {
        this.theme = this.themeSelect.value;
        localStorage.setItem('lanChatTheme', this.theme);
        this.applyTheme();
        this.setStatus('Theme changed');
    }

    showSettings() {
        this.settingsModal.classList.add('show');
        this.usernameInput.focus();
    }

    hideSettings() {
        this.settingsModal.classList.remove('show');
    }

    showPeers() {
        this.peersModal.classList.add('show');
        this.loadPeers();
    }

    hidePeers() {
        this.peersModal.classList.remove('show');
    }

    async sendMessage() {
        if (!this.username) {
            this.showSettings();
            return;
        }

        const text = this.messageInput.value.trim();
        if (!text) return;

        if (text.length > 1000) {
            alert('Message too long. Maximum 1000 characters.');
            return;
        }

        try {
            this.sendBtn.disabled = true;
            const response = await fetch('/messages', {  // Fixed: Removed /api/ prefix
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({
                    user: this.username,
                    message: text
                })
            });

            if (response.ok) {
                this.messageInput.value = '';
                this.loadMessages();
                this.setStatus('Message sent');
            } else {
                throw new Error(`Server error: ${response.status}`);
            }
        } catch (error) {
            console.error('Failed to send message:', error);
            this.setStatus('Failed to send message', 'error');
        } finally {
            this.sendBtn.disabled = false;
            this.messageInput.focus();
        }
    }

    async loadMessages() {
        try {
            const response = await fetch('/messages');  // Fixed: Removed /api/ prefix
            if (!response.ok) {
                throw new Error(`Server error: ${response.status}`);
            }

            const messages = await response.json();
            
            // Only update if message count changed
            if (messages.length !== this.lastMessageCount) {
                this.displayMessages(messages);
                this.lastMessageCount = messages.length;
                
                if (messages.length > 0) {
                    this.setStatus(`${messages.length} messages loaded`);
                } else {
                    this.setStatus('No messages yet');
                }
            }
        } catch (error) {
            console.error('Failed to load messages:', error);
            this.setStatus('Failed to load messages - Check connection or server', 'error');
        }
    }

    displayMessages(messages) {
        // Clear welcome message
        const welcomeMsg = this.messagesContainer.querySelector('.welcome-message');
        if (welcomeMsg && messages.length > 0) {
            welcomeMsg.remove();
        }

        // Clear existing messages
        this.messagesContainer.querySelectorAll('.message').forEach(msg => msg.remove());

        // Add new messages
        messages.forEach(message => {
            const messageEl = this.createMessageElement(message);
            this.messagesContainer.appendChild(messageEl);
        });

        // Scroll to bottom
        this.scrollToBottom();
    }

    createMessageElement(message) {
        const messageDiv = document.createElement('div');
        const isOwn = message.user === this.username;
        messageDiv.className = `message ${isOwn ? 'own' : 'other'}`;

        const bubbleDiv = document.createElement('div');
        bubbleDiv.className = 'message-bubble';

        if (!isOwn) {
            const headerDiv = document.createElement('div');
            headerDiv.className = 'message-header';
            headerDiv.textContent = message.user;
            bubbleDiv.appendChild(headerDiv);
        }

        const textDiv = document.createElement('div');
        textDiv.className = 'message-text';
        textDiv.textContent = message.message;
        bubbleDiv.appendChild(textDiv);

        const timeDiv = document.createElement('div');
        timeDiv.className = 'message-time';
        timeDiv.textContent = this.formatTime(message.timestamp);
        bubbleDiv.appendChild(timeDiv);

        messageDiv.appendChild(bubbleDiv);
        return messageDiv;
    }

    formatTime(timestamp) {
        try {
            const date = new Date(timestamp);
            return date.toLocaleTimeString('en-US', {
                hour: '2-digit',
                minute: '2-digit',
                hour12: false
            });
        } catch (error) {
            return timestamp; // Fallback to original string
        }
    }

    scrollToBottom() {
        this.messagesContainer.scrollTop = this.messagesContainer.scrollHeight;
    }

    async loadPeers() {
        try {
            const response = await fetch('/peers');  // Fixed: Removed /api/ prefix
            if (!response.ok) {
                throw new Error(`Server error: ${response.status}`);
            }

            const peers = await response.json();
            this.displayPeers(peers);
            this.updatePeerCount(peers.length);
        } catch (error) {
            console.error('Failed to load peers:', error);
            this.peersList.innerHTML = '<p style="color: #e53e3e;">Failed to load peers - Check connection or server</p>';
        }
    }

    displayPeers(peers) {
        if (peers.length === 0) {
            this.peersList.innerHTML = '<p>No peers found. Make sure other instances are running on the network.</p>';
            return;
        }

        this.peersList.innerHTML = '';
        peers.forEach(peer => {
            const peerDiv = document.createElement('div');
            peerDiv.className = 'peer-item';

            const avatarDiv = document.createElement('div');
            avatarDiv.className = 'peer-avatar';
            avatarDiv.textContent = peer.id.charAt(peer.id.length - 1).toUpperCase();

            const infoDiv = document.createElement('div');
            infoDiv.className = 'peer-info';

            const nameH3 = document.createElement('h3');
            nameH3.textContent = peer.id || 'Unknown';

            const addressP = document.createElement('p');
            addressP.textContent = peer.address || 'Unknown address';

            infoDiv.appendChild(nameH3);
            infoDiv.appendChild(addressP);
            peerDiv.appendChild(avatarDiv);
            peerDiv.appendChild(infoDiv);
            this.peersList.appendChild(peerDiv);
        });
    }

    updatePeerCount(count) {
        this.peerCount.textContent = `${count} peer${count !== 1 ? 's' : ''} online`;
    }

    async clearMessages() {
        if (!confirm('Are you sure you want to clear all messages? This cannot be undone.')) {
            return;
        }

        try {
            const response = await fetch('/clear', { method: 'POST' });  // Assumes you add /clear endpoint in backend
            if (response.ok) {
                this.setStatus('Messages cleared');
                this.loadMessages();
            } else {
                throw new Error(`Server error: ${response.status}`);
            }
        } catch (error) {
            console.error('Failed to clear messages:', error);
            this.setStatus('Failed to clear messages', 'error');
        } finally {
            this.hideSettings();
        }
    }

    setStatus(message, type = 'info') {
        this.statusText.textContent = message;
        this.statusText.style.color = type === 'error' ? '#e53e3e' : '#8696a0';
        
        // Clear status after 3 seconds unless it's an error
        setTimeout(() => {
            if (type !== 'error') {
                this.statusText.textContent = 'Ready';
                this.statusText.style.color = '#8696a0';
            }
        }, 3000);
    }

    startPolling() {
        // Load initial data
        this.loadMessages();
        this.loadPeers();

        // Poll for new messages every 5 seconds (increased for efficiency)
        this.pollInterval = setInterval(() => {
            this.loadMessages();
        }, 5000);

        // Poll for peers every 30 seconds
        this.peerPollInterval = setInterval(() => {
            this.loadPeers();
        }, 30000);
    }

    stopPolling() {
        if (this.pollInterval) {
            clearInterval(this.pollInterval);
            this.pollInterval = null;
        }
        if (this.peerPollInterval) {
            clearInterval(this.peerPollInterval);
            this.peerPollInterval = null;
        }
    }
}

// Initialize the application when DOM is loaded
document.addEventListener('DOMContentLoaded', () => {
    window.lanChat = new LanChat();
});

// Handle page visibility for efficient polling
document.addEventListener('visibilitychange', () => {
    if (window.lanChat) {
        if (document.hidden) {
            window.lanChat.stopPolling();
        } else {
            window.lanChat.startPolling();
        }
    }
});

// Handle browser close/refresh
window.addEventListener('beforeunload', () => {
    if (window.lanChat) {
        window.lanChat.stopPolling();
    }
});