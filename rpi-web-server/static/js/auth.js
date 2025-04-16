/**
 * Web of Things Platform - Authentication Scripts
 * Handles login, signup, token management, and redirect
 */

document.addEventListener('DOMContentLoaded', function() {
    // DOM Elements
    const loginForm = document.getElementById('login-form');
    const signupForm = document.getElementById('signup-form');
    const errorMessage = document.getElementById('error-message');
    const passwordToggles = document.querySelectorAll('.toggle-password');
    
    // Toggle password visibility
    passwordToggles.forEach(toggle => {
        toggle.addEventListener('click', function() {
            const input = this.previousElementSibling;
            const type = input.getAttribute('type') === 'password' ? 'text' : 'password';
            input.setAttribute('type', type);
            
            // Toggle eye icon
            const icon = this.querySelector('i');
            icon.classList.toggle('fa-eye');
            icon.classList.toggle('fa-eye-slash');
        });
    });
    
    // Handle login form submission
    if (loginForm) {
        loginForm.addEventListener('submit', async function(e) {
            e.preventDefault();
            const username = document.getElementById('username').value;
            const password = document.getElementById('password').value;
            const rememberMe = document.getElementById('remember-me')?.checked || false;
            
            // Show loading state
            const submitBtn = this.querySelector('button[type="submit"]');
            submitBtn.disabled = true;
            submitBtn.querySelector('span').style.opacity = '0';
            submitBtn.querySelector('.spinner').classList.add('show');
            
            try {
                const response = await login(username, password);
                
                // Store tokens
                storeTokens(response.access_token, response.refresh_token, rememberMe);
                
                // Redirect to dashboard
                window.location.href = '/index.html';
            } catch (error) {
                showError(error.message || 'Login failed. Please check your credentials.');
                
                // Reset loading state
                submitBtn.disabled = false;
                submitBtn.querySelector('span').style.opacity = '1';
                submitBtn.querySelector('.spinner').classList.remove('show');
            }
        });
    }
    
    // Handle signup form submission
    if (signupForm) {
        signupForm.addEventListener('submit', async function(e) {
            e.preventDefault();
            const username = document.getElementById('username').value;
            const email = document.getElementById('email').value;
            const fullName = document.getElementById('full-name').value;
            const password = document.getElementById('password').value;
            const confirmPassword = document.getElementById('confirm-password').value;
            const agreeTerms = document.getElementById('agree-terms').checked;
            
            // Validate form
            if (password !== confirmPassword) {
                showError('Passwords do not match');
                return;
            }
            
            if (!agreeTerms) {
                showError('You must agree to the Terms of Service and Privacy Policy');
                return;
            }
            
            // Show loading state
            const submitBtn = this.querySelector('button[type="submit"]');
            submitBtn.disabled = true;
            submitBtn.querySelector('span').style.opacity = '0';
            submitBtn.querySelector('.spinner').classList.add('show');
            
            try {
                // First get admin access token (for registration)
                const adminResponse = await login('admin', 'admin123'); // Using default credentials
                
                // Register new user
                await registerUser(
                    username, 
                    email, 
                    password, 
                    fullName, 
                    adminResponse.access_token
                );
                
                // Show success message
                errorMessage.textContent = 'Account created successfully! Redirecting to login...';
                errorMessage.classList.remove('error-message');
                errorMessage.classList.add('success-message');
                errorMessage.classList.add('show');
                
                // Redirect to login after 2 seconds
                setTimeout(() => {
                    window.location.href = '/login.html';
                }, 2000);
            } catch (error) {
                showError(error.message || 'Registration failed. Please try again.');
                
                // Reset loading state
                submitBtn.disabled = false;
                submitBtn.querySelector('span').style.opacity = '1';
                submitBtn.querySelector('.spinner').classList.remove('show');
            }
        });
    }
    
    // Check if user is already logged in
    checkAuthentication();
});

/**
 * Login function - sends credentials to API
 * @param {string} username - username
 * @param {string} password - password
 * @returns {Promise} - Promise with token response
 */
async function login(username, password) {
    const formData = new URLSearchParams();
    formData.append('username', username);
    formData.append('password', password);
    
    const response = await fetch('/api/auth/token', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/x-www-form-urlencoded',
        },
        body: formData
    });
    
    const data = await response.json();
    
    if (!response.ok) {
        throw new Error(data.detail || 'Authentication failed');
    }
    
    return data;
}

/**
 * Register a new user
 * @param {string} username - username
 * @param {string} email - email
 * @param {string} password - password
 * @param {string} fullName - full name (optional)
 * @param {string} adminToken - admin token for authorization
 * @returns {Promise} - Promise with registration response
 */
async function registerUser(username, email, password, fullName, adminToken) {
    const response = await fetch('/api/auth/register', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
            'Authorization': `Bearer ${adminToken}`
        },
        body: JSON.stringify({
            username,
            email,
            password,
            full_name: fullName
        })
    });
    
    const data = await response.json();
    
    if (!response.ok) {
        throw new Error(data.detail || 'Registration failed');
    }
    
    return data;
}

/**
 * Store tokens in localStorage or sessionStorage
 * @param {string} accessToken - JWT access token
 * @param {string} refreshToken - Refresh token
 * @param {boolean} rememberMe - Whether to store in localStorage
 */
function storeTokens(accessToken, refreshToken, rememberMe) {
    const storage = rememberMe ? localStorage : sessionStorage;
    
    storage.setItem('access_token', accessToken);
    storage.setItem('refresh_token', refreshToken);
    
    // Set a cookie for middleware authentication
    document.cookie = `access_token=${accessToken}; path=/; max-age=${rememberMe ? 86400 * 30 : 86400}; SameSite=Strict`;
}

/**
 * Display error message
 * @param {string} message - Error message to display
 */
function showError(message) {
    const errorElement = document.getElementById('error-message');
    errorElement.textContent = message;
    errorElement.classList.add('show');
    
    // Hide after 5 seconds
    setTimeout(() => {
        errorElement.classList.remove('show');
    }, 5000);
}

/**
 * Check if user is authenticated and redirect if needed
 */
function checkAuthentication() {
    // Get current page
    const currentPage = window.location.pathname;
    
    // Check for token
    const accessToken = localStorage.getItem('access_token') || sessionStorage.getItem('access_token');
    
    if (accessToken) {
        // User has token - check if on login/signup page
        if (currentPage.includes('login.html') || currentPage.includes('signup.html')) {
            // Redirect to dashboard
            window.location.href = '/index.html';
        }
    } else {
        // No token - check if on protected page
        if (currentPage === '/' || currentPage.includes('index.html')) {
            // Will be handled by the middleware, but add additional check
            checkTokenValidity();
        }
    }
}

/**
 * Check if token is valid
 */
async function checkTokenValidity() {
    const accessToken = localStorage.getItem('access_token') || sessionStorage.getItem('access_token');
    
    if (!accessToken) {
        return false;
    }
    
    try {
        const response = await fetch('/api/auth/me', {
            method: 'GET',
            headers: {
                'Authorization': `Bearer ${accessToken}`
            }
        });
        
        if (!response.ok) {
            // Token is invalid - try to refresh
            await refreshAccessToken();
        }
        
        return true;
    } catch (error) {
        console.error('Error checking token validity:', error);
        return false;
    }
}

/**
 * Refresh the access token using refresh token
 */
async function refreshAccessToken() {
    const refreshToken = localStorage.getItem('refresh_token') || sessionStorage.getItem('refresh_token');
    
    if (!refreshToken) {
        // No refresh token - redirect to login
        logout();
        return;
    }
    
    try {
        const response = await fetch('/api/auth/refresh', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({
                refresh_token: refreshToken
            })
        });
        
        if (!response.ok) {
            throw new Error('Failed to refresh token');
        }
        
        const data = await response.json();
        
        // Store new tokens
        const storage = localStorage.getItem('refresh_token') ? localStorage : sessionStorage;
        storage.setItem('access_token', data.access_token);
        storage.setItem('refresh_token', data.refresh_token);
        
        // Update cookie
        const maxAge = localStorage.getItem('refresh_token') ? 86400 * 30 : 86400;
        document.cookie = `access_token=${data.access_token}; path=/; max-age=${maxAge}; SameSite=Strict`;
        
        return true;
    } catch (error) {
        console.error('Error refreshing token:', error);
        logout();
        return false;
    }
}

/**
 * Logout user - clear tokens and redirect
 */
function logout() {
    // Clear tokens
    localStorage.removeItem('access_token');
    localStorage.removeItem('refresh_token');
    sessionStorage.removeItem('access_token');
    sessionStorage.removeItem('refresh_token');
    
    // Clear cookie
    document.cookie = 'access_token=; path=/; max-age=0; SameSite=Strict';
    
    // Redirect to login
    window.location.href = '/login.html';
}

// Expose logout function globally
window.logout = logout;
